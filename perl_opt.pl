#!/bin/perl

#while(<>) {print;}
#exit 0;

$pppv = "";
$pprv = "";
$prev = "";
$opt = 0;

$NOREG = "NOREG";

$D_REGS = $A_REGS = $NOREG;

$offset = 0;
&prep_addr(_1, _2, _3, _4, _5, _6, _7, regs);
&prep_addr(sbitmap, sdelta1, sdelta2, sdelta3, sdelta4, video_ram, drawfunc);
&prep_addr(
GE7DA, GE7DB, GE7DC, GE7DD, 
GE7E5, GE7E6, GE7E7,
ORA, ORB, DDRA, DDRB, CRA, CRB,
CRC, PRC, 
ORA2, ORB2, DDR12, DDRB2, CRA2, CRB2, port0,
MON0_7E7, MON1_7E7, 
mem_ptr, XXX_mem_image,
perl_opt_dummy);

#&prep_addr2(ROM, 16,MON, 8,EXT, 4,RAM, 128);

$local_lbl = "lbl000";

while(<>) {
	next if /^#(NO_)?APP/;

	if(s/LOCALLABEL/$local_lbl/ && /$local_lbl:/) {
		$local_lbl++;
		$_ = "$local_lbl:\n";
	}
	
	if(/reg_(.*)_([dD].*):/) {
		$REG{$1} = $2; 
		$D_REGS = $D_REGS ne $NOREG?"$D_REGS|$2":"$2";
	} elsif(/reg_(.*)_([aA].*):/) {
		$REG{$1} = $2;
		$A_REGS = $A_REGS ne $NOREG?"$A_REGS|$2":"$2";
	}

	######################################################################
	# Optims generales
	######################################################################

	# pour rendre le code plus propre
	s/\#--/\#/;
	
	# movex d0,d0
	if(/move. (d\d),\1/) {
		$_ = "";
		++$opt;
	}

	# jbsr XXXX; rts ==> jbra XXXX
	if(/rts/ && $prev=~/jbsr/) {
		$prev =~ s/jbsr/jra/;
		++$opt;
		$_ = "";
	}

	# move ADR,*; move ADR,Dx ==> move ADR,Dx; move Dx,*
	#
	# on utilise Dx autant qu'on peut.
	if(/move(.) (_.+),(d\d)/ && ($a=$2,$d=$3,1) && $prev=~/move$1 $2,/) {
		$prev =~ s/$a/$d/; $t = $prev; $prev=$_; $_=$t;
		++$opt;
	}

	# jbsr a0@; movel sp@+,An; rts ==> movel sp@+,An; jbra a0@
	#
	# Car An pas utilise (ou est protege) dans le code pointe par
	# a0 (en effet, si An etait un scratch reg, il ne serait pas
	# restaure, donc An est un registre non scratch et il est protege
	# dans toutes les sous-routines de ce projet, donc on peut le
	# restaurer plus tot et economiser un rts).
	if(/rts/ && $prev=~/movel sp\@\+,a\d/ && $pprv=~/jbsr a0\@/) {
		$pprv = $prev;
		$prev = "\tjbra a0@\n";
		$_    = "";
		++$opt;
	}

	# addw Dm,Dp; movel Dp,Dm ==> addw Dp,Dm
	if(/addw (d\d),(d\d); movel \2,\1/) {
		$_ = "\taddw $2,$1\n";
		++$opt;
	}
	
	if(/tst(.) (.*)/) {
		$b=$1; $a=$2; $a=~s/[()@]/\\$&/g;
		if($a!~/^a\d$/ && $prev=~/move$b $a,|move$b .*,$a|(add|sub|and|or|eor)$b .*,$a|(com|neg)$b $a/) {
	        	$_ = "";
		        ++$opt;
		}
	}

	######################################################################
	# Optims particuliere a TEO
	######################################################################
	# on assume que si un byte est demandee en entree a une fonction
	# le "and" sera fait dans la fonction.
	if(/j(bsr|ra) a0[@]/ && $prev=~/andl [#]0xFF,d1/) {
		$prev = "";
		++$opt;
	}
	if(/j(bsr|ra) a0[@]/ && $prev=~/moveb .*,d1/ && $pprv=~/clrl d1/) {
		$pprv = "";
		++$opt;
	}

	# addl _sdelta,*; clrl d0 ==> addl _sdelta,*
	#
	# optim un peu hasardeuse, mais si sdelta n'est utilise que dans
	# amiga/graph.c ca doit marcher (on vire le clr intermediaire car
	# la partie haute est deja nulle).
	if(/clrl d0/ && $prev=~/addl _sdelta/) {
		++$opt;
		$_ = "";
	}

	# INST Am@(Dx:l:4),Ap; clrl Dx; moveb *,Dx ==> INST Am@(Dx:l:4),Ap; move[bw] *,Dx
	#
	# ici c'est pareil.. on vire le clrl intermediaire car la partie haute
        # de Dx est probablement nulle.
	if(/move[bw] .+,(d\d)/ && ($d=$1) && $prev=~/clrl $d/ && $pprv=~/a\d@\($d:l:4\),a\d/) {
		$prev = $_; #"nop\n";
		$_ = "";
		++$opt;
	}

	# sne Dx; negb Dx; moveb Dx,_res+3 ==> sne Dx; moveb Dx, _res+3
	#
	# neg intermediaire est elimine car seul la nullite de Dx.b importe
	if(/moveb (d\d),_res\+3/ && ($d=$1) && $prev=~/negb $d/ && $pprv=~/sne $d/) {
		++$opt;
		$pprv = "\tsne _res\+3\n";
		$prev = "";
		$_    = "";
	}
	
	# clrl (_A ou _B);  moveb *,(_A ou _B)+3 ==> moveb *,(_A ou _B)+3
	#
	# on vire le clrl car _A ou _B ont deja leur partie haute nulle.
	if(/moveb .*,(_[AB])\+3/ && $prev=~/clrl $1/) {
		$prev = "";
		++$opt;
	}
	# idem pour _X _Y _S _U
	if(/movew .*,(_[XYSU])\+2/ && $prev=~/clr[lw] $1/) {
		$prev="";
		++$opt;
	}
	# idem pour _M et _m qui sont stoques dans d6 et d7
	if(/movew .*,($D_REGS)/ && $prev=~/clrl $1|moveq \#0,$1/) {
		$prev="";
		++$opt;
	}

	# on accede sur 16bits _X _Y _U _S.
	if(/l\s+[^_].*,_[USXY]$/) {
		s/l (.*)/w $1\+2/;
		++$opt;
	}
 
	# 8 bits pour _A, _B, _m1, _m2, _ovfl, _sign
	s/l(\s+[^_].*,_[AB])$/b$1\+3/ && ++$opt;
	s/movel _ovfl,_m1/moveb _ovfl\+3,_m1\+3/ && ++$opt;
	s/l( d\d,_m[12])/b$1\+3/ && ++$opt;
	s/l( d\d,_(sign|ovfl))/b$1\+3/ && ++$opt;

	# bfextu *,Dm;movel An@(Dm:l:4),*; clrl Dp; move *,Dp 
	# ==> 
	# bfextu *,Dp;movel An@(Dp:l:4),*; move *,Dp 
	#
	# on calcule tout sur Dp=d6 ou d7 pour effacer le clrl.
	if(/move[bw] .*,($D_REGS)/ && ($d=$1) && $prev=~/clrl $d/ && 
           $pprv=~/movel .*\@\((d[01]):l:4\),/ && ($dd=$1) && 
           $pppv=~/bfextu .*,$dd/) {
		$pprv=~ s/$dd/$d/;
		$pppv=~ s/$dd/$d/;
		$prev="";
		++$opt;
	}
		
	# bfextu _res+2{#7:#1},Dm ==> moveq #1,Dm; andb _res+2,Dm
	#
	# ca sert pour l'extraction de la cary dans _res.
	if(/bfextu _res\+2{\#7:\#1},(d\d)/) {
		$_="\tmoveq #1,$1; andb _res+2,$1\n";
		++$opt;
	}
	
	# moveX XXX,Dm; andX #n,Dm ==> moveq #n,Dm; andX XXX,Dm
	if(/andw #(\d+),(d\d)/ && ($n=$1, $d=$2, $n>0 && $n<128) && 
	   $prev =~ /movew (.*),$d/ && ($dd=$1)) {
	   $prev = "\tmoveq #$n,$d\n";
	   s/#$n,/$dd,/;
	}

	######################################################################
	# Optims pas interessantes
	######################################################################

# ca change les flags ca, non ?	
#	if(/lsll \#8,(d[67])/ && ($d=$1) && $prev=~/moveb .*,$d/) {
#		$prev=~ s/moveb/movew/;
#		$_="";
#		++$opt;
#	}
#	s/orb (.*\@.*,d[67])/moveb $1/;
	
#	if(/lsll \#8,(d[67])/ && ($d=$1) && $prev=~/movel _DP,$d/) {
#		$prev="\tmovew _DP+3,$d\n";
#		$_="";
#		++$opt;
#	}

#	if(s/movew (_[USXY])\+2,/movel $1,/) {
#		++$opt;
#	}

#	if(/lsll \#8,(d[67])/ && ($d=$1) && $prev=~/moveb .*,$d/ && $pprv=~/clrl $d/) {
##		$pprv="";
##		$_="\tlslw #8,$d\n";
#		$prev=~s/moveb/movew/;
#		$_="";
#		++$opt;
#	}

#	if(/extbl d[67]/) {s/bl/w/;}
#	if(/l [^_].*,d[67]/) {s/l /w /;}
	
#	if(/clrl (d\d)/ && ($d=$1) && $prev=~/\@\(d0:l:4\),a0/ && $pprv=~/bfextu .*{\#16:\#4},d0/) {
#		$pprv=~ s/d0/$d/;
#		$prev=~ s/d0/$d/;
#		$_="";
#		++$opt;
#	}

	# ca ca accelere sur 040 je crois
#	if(/rts/) {
#		s/rts/movel sp@+,a0; jmp a0@/;
#	}

#	if(/bfextu (d\d){\#16:\#4},(d\d)/) {
#		$_="\tmovew $1,$2; lsrw #6,$2; lsrw #6,$2\n";
#		++$opt;
#	}

	# on remplace les acces 32bits par un offset sur REG{MAPPER}.
	if($REG{MAPPER} && ($pppv !~ /comm|stab/)) {
		$pppv =~ s/([\s,])_([^\s,;{}]+)/$1.&r_addr($2)/eg;
	}
	print $pppv;

	$pppv = $pprv;
	$pprv = $prev;
	$prev = $_;
}
print $pppv;
print $pprv;
print $prev;
print STDERR "opt=$opt\n";

sub r_addr {
	local($adr) = @_;
	local($suf);

#print STDERR ">>> $adr\n";
	
	$adr =~ s/[+-]\d+$/$suf=$&,""/e;
	$adr = defined($offset{$adr})?(++$opt,"$REG{MAPPER}@($offset{$adr}$suf)"):"_$adr$suf";
#print STDERR "<<< $adr\n";
	return $adr;
}

sub prep_addr {
	foreach $_ (@_) { $offset{$_} = ($offset -= 4); }
}
sub prep_addr2 {
	local($i) = 0;
	for($i=0;$i< $#_; $i += 2) { $offset{$_[$i]} = ($offset -= $_[$i+1]); }
}
