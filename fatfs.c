#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <linux/msdos_fs.h>
#include <math.h>
#include <stdint.h>

#define SECTORSIZE 512   //bytes
#define BLOCKSIZE  4096  // bytes - do not change this value
unsigned char rootblock[BLOCKSIZE];
char diskname[48]; 
int  disk_fd; 
char parameter[48];
struct fat_boot_sector *fat;
struct msdos_dir_entry *dir;
struct msdos_dir_entry *rootDirectory;
unsigned char volumesector[SECTORSIZE]; 
unsigned int clusterBegin;
unsigned int root;
int get_sector (unsigned char *buf, int snum)
{
	off_t offset; 
	int n; 
	offset = snum * SECTORSIZE; 
	lseek (disk_fd, offset, SEEK_SET); 
	n  = read (disk_fd, buf, SECTORSIZE); 
	if (n == SECTORSIZE) 
		return (0); 
	else {
		printf ("sector number %d invalid or read error.\n", snum); 
		exit (1); 
	}
}

void print_sector (unsigned char *s)
{
	int i;

	for (i = 0; i < SECTORSIZE; ++i) {
		printf ("%02x ", (unsigned char) s[i]); 
		if ((i+1) % 16 == 0)
			printf ("\n"); 
	}
	printf ("\n");
}


int main(int argc, char *argv[])
{	
	if (argc < 4) {
		printf ("wrong usage\n"); 
		exit(1); 
	}
	
	strcpy(diskname, argv[1]);
	strcpy(parameter, argv[3]); 
        disk_fd = open (diskname, O_RDWR); 
	if (disk_fd < 0) {
		printf ("could not open the disk image\n"); 
		exit(1); 
	} 

	if(strcmp(parameter,"volumeinfo") == 0){
		printf("Volume Information\n");
		get_sector(volumesector,0);
		fat = (struct fat_boot_sector*)volumesector;
		printf("Volume ID: ");
		for(int i = 0; i <= 3;i++){
			printf("%02x", fat->fat32.vol_id[i]);
		}
		printf("\n");
		printf("No of FAT: %lu\n", fat->fats);
		printf("Sector per Cluster: %lu\n", fat->sec_per_clus);
		printf("Sector per FAT: %lu\n", fat->fat_length);
		printf("System ID: ");
		for(int i = 0; i <= 7;i++){
			printf("%02x", fat->system_id[i]);
		}
		printf("\n");
		printf("System ID Name: ");
		for(int i = 0; i <= 7;i++){
			printf("%c", fat->system_id[i]);
		}
		printf("\n");
		printf("Number of Sectors: ");
		for(int i = 0; i <= 1;i++){
			printf("%d", fat->sectors[i]);
		}
		printf("\n");
		printf("Root Cluster: %d\n", fat->fat32.root_cluster);
		printf("Dir Entrie: %x\n",fat->dir_entries[0]);
		printf("Dir Entrie: %x\n",fat->dir_entries[1]);

	}//End of volume info

	if(strcmp(parameter,"rootdir") == 0){
		//printf("Root Directory\n");
		get_sector(volumesector,0);
		fat = (struct fat_boot_sector*)volumesector;
		clusterBegin = fat->reserved + (unsigned int)(fat->fats) * ((fat->fat32).length);
		root = clusterBegin + ((fat->fat32).root_cluster - 2) * (unsigned int)(fat->sec_per_clus);
		
		for(int i = 0; i < 8; i++){
        	get_sector(rootblock + SECTORSIZE * i, root + i);
		}
		rootDirectory = (struct msdos_dir_entry *)rootblock;
		int i = 0 ;
		struct msdos_dir_entry *dirEntry;
		while((rootDirectory + i)->name[0] != 0x00){
		
			dirEntry = rootDirectory + i ;
			printf("%s\n",dirEntry->name);
			i++;

		}		
	}//End of root dir
	
	if(strcmp(parameter,"blocks") == 0){
		if(argc < 5){
			printf("specify file name!\n");
			exit(1);
		}
		//printf("blocks\n");
		char file[11];
		strcpy(file, argv[4]);
		//printf("searching for %s\n",file);
		get_sector(volumesector,0);
		fat = (struct fat_boot_sector*)volumesector;
		clusterBegin = fat->reserved + (unsigned int)(fat->fats) * ((fat->fat32).length);
		root = clusterBegin + ((fat->fat32).root_cluster - 2) * (unsigned int)(fat->sec_per_clus);
		
		for(int i = 0; i < 8; i++){
        	get_sector(rootblock + SECTORSIZE * i, root + i);
		}
		rootDirectory = (struct msdos_dir_entry *)rootblock;
		int i = 0 ;
		struct msdos_dir_entry *dirEntry;
		char name[9];// name before ext and null terminate
		char ext[4]; // ext with null
		char fullname[10]; // full
		
		while((rootDirectory + i)->name[0] != 0x00){
			strncpy(name,(char*)((rootDirectory+i)->name),8);
			strncpy(ext,(char*)((rootDirectory+i)->name)+8,3);
			name[8]='\0';
			ext[3]= '\0';
			for(int i =7;i>-1 && name[i]==0x20;i--){
				name[i]='\0';
			}			
			for(int i =3;i>-1 && name[i]==0x20;i--){
				name[i]='\0';
			}
			strcpy(fullname,name);
			fullname[strlen(name)]='.';
			strcpy(fullname+strlen(name)+1,ext);
			if(strcasecmp(fullname,file)==0){//matching the input
				printf("%s\n",(rootDirectory+i)->name);
				if((rootDirectory+i)->size == 0){
					printf("file size is 0!");                        		
					exit(1);
				}
				uint32_t in = ((rootDirectory+i)->starthi)*256 + (rootDirectory+i)->start;
				while(in != EOF_FAT32){
					printf("%u : %x\n",in,in);
					unsigned char block[SECTORSIZE];
					get_sector(block,fat->reserved +in/(SECTORSIZE/sizeof(int32_t)));
					uint32_t *nextblock = (uint32_t*)block;
					in = nextblock[in%(SECTORSIZE/sizeof(int32_t))];	
				}
								
			
			}			

		i++;	
		}



	}//End of blocks	
	
	close (disk_fd); 

	return (0); 
}

