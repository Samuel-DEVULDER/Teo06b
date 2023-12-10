/*

	VERSION     : 2.0
	MODULE      : diskio.h
	Créé par    : Gilles FETIS
	Modifié par : Eric Botcazou

*/

#ifndef DISKIO_H
#define DISKIO_H

extern char SAPfname[2][1024]; /* sam: comme ca la GUI peut connaitre le nom */

extern void LoadTDS(char * name,int side);
extern void SaveTDS(char * name,int side);
extern void LoadSAP(char * name,int side);
extern void DiskIO(void);
extern void InitDiskCtrl(void);
extern void ReadSector(void);
extern void WriteSector(void);
#endif
