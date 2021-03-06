/*
	This file is part of SHMUP.

    SHMUP is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    SHMUP is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
*/    
/*
 *  entities.c
 *  dEngine
 *
 *  Created by fabien sanglard on 11/09/09.
 *  Copyright 2009 Memset software Inc. All rights reserved.
 *
 */

#include "entities.h"
#include <float.h>
#include "renderer.h"

//Variable used to make entities stick to screen
float distanceZFromCamera; 
float widthAtDistance;
float heightAtDistance;
matrix_t cameraInvRot;

typedef struct md5_bucket_t {
	md5_mesh_t* mesh ;
	char* name;
	struct md5_bucket_t *next;
} md5_bucket_t;

#define SIZE_MD5_HASHTABLE 256
md5_bucket_t** md5_hashtable;

void ENT_InitCacheSystem(void)
{
	md5_hashtable = calloc(SIZE_MD5_HASHTABLE,sizeof(md5_bucket_t*));
}



void ENT_DumpEntityCache(void)
{
	int i,j;
	md5_bucket_t* currentBucket;
	md5_bucket_t* cursorBucket;
	md5_mesh_t* mesh ;
	vertex_t* currentVertex;

	for(j=0 ; j < SIZE_MD5_HASHTABLE ; j++)
	{
		currentBucket = md5_hashtable[j];
		cursorBucket = currentBucket;
		while(cursorBucket != NULL)
		{
			Log_Printf("Loc[%d]-Dumping mesh '%s'.\n",j,cursorBucket->name);
			mesh= cursorBucket->mesh;

			Log_Printf("Listing Vertices.\n");
			if (mesh->memLocation == MD5_MEMLOC_RAM)
			{
				currentVertex = mesh->vertexArray;
				

				for (i=0; i < mesh->numVertices ; i++,currentVertex++) 
				{
					Log_Printf("vertex: %d/%d  (norm: %hd , %hd , %hd ) (text: %hd , %hd ) (pos: %f , %f , %f )\n",
						   i,
						   mesh->numVertices-1,   
						   currentVertex->normal[0],
						   currentVertex->normal[1],
						   currentVertex->normal[2],
						   currentVertex->text[0],
						   currentVertex->text[1],
						   currentVertex->pos[0],
						   currentVertex->pos[1],
						   currentVertex->pos[2]	   
						   );
				}
			}
			else
			{
				if (mesh->memLocation == MD5_MEMLOC_VRAM)
					Log_Printf("Vertices on GPU.\n");
				else
					Log_Printf("Vertices on DISK.\n");
			}

			Log_Printf("Listing indices.\n");
			if (mesh->indicesMemoryLoc == MD5_MEMLOC_RAM)
			{
				for(i=0;  i < mesh->numIndices ; i++)
				{
					Log_Printf("indice: %d/%d, vertex: %hu   (norm: %hd , %hd , %hd ) (text: %hd , %hd ) (pos: %f , %f , %f )\n",
						   i,
						   mesh->numIndices-1,   
							mesh->indices[i],
						   mesh->vertexArray[mesh->indices[i]].normal[0],
						   mesh->vertexArray[mesh->indices[i]].normal[1],
						   mesh->vertexArray[mesh->indices[i]].normal[2],
						   mesh->vertexArray[mesh->indices[i]].text[0],
						   mesh->vertexArray[mesh->indices[i]].text[1],
						   mesh->vertexArray[mesh->indices[i]].pos[0],
						   mesh->vertexArray[mesh->indices[i]].pos[1],
						   mesh->vertexArray[mesh->indices[i]].pos[2]	 
						   );
				}
			}
			else
			{
				if (mesh->memLocation == MD5_MEMLOC_VRAM)
						Log_Printf("Indices on GPU.\n");
					else
						Log_Printf("Indices on DISK.\n");
			}
			cursorBucket = cursorBucket->next;
		}
	}
}


uint Get_MD5_HashValue( const char *string )
{
	uint hash = *string;
	
	if( hash )
	{
		for( string += 1; *string != '\0'; ++string )
		{
			hash = (hash << 5) - hash + *string;
		}
	}
	
	return hash % SIZE_MD5_HASHTABLE;
}

void ENT_Put(md5_mesh_t* mesh, const char* meshName)
{
	md5_bucket_t* bucket;
	unsigned int hashValue ; 
	
	hashValue= Get_MD5_HashValue(meshName);
	
	bucket = md5_hashtable[hashValue] ;
	
	if ( bucket == NULL)
	{
		bucket = (md5_bucket_t*)calloc(1,sizeof(md5_bucket_t));
		
		md5_hashtable[hashValue] = bucket;
	}
	else
	{		
		while (bucket->next)
			bucket = bucket->next;
		
		bucket->next = (md5_bucket_t*)calloc(1,sizeof(md5_bucket_t));
		bucket = bucket->next;
	}
	
	
	bucket->mesh = mesh;
	bucket->name = calloc(strlen(meshName)+1, sizeof(char));
	strcpy(bucket->name, meshName);
	bucket->next = NULL;
}

md5_mesh_t*  ENT_Get(const char* meshName)
{ 
	unsigned int hashValue ;
	md5_bucket_t* bucket;
	
	hashValue= Get_MD5_HashValue(meshName);
	
	bucket = md5_hashtable[hashValue];
	
	if (bucket == NULL)
		return NULL;
	
	while (1)
	{
		if (strcmp(bucket->name,meshName))
		{
			if (bucket->next != NULL)
				bucket = bucket->next;
			else
				return NULL;
		}
		else
			return bucket->mesh;
	}
	
	//return NULL;
}




void ENT_ClearModelsLibrary(void)
{
	int i;
	md5_bucket_t* curr;
	md5_bucket_t* prev;
	md5_bucket_t* toDelete;
	
	for (i=0; i < SIZE_MD5_HASHTABLE; i++) 
	{  
		prev = NULL;
		
		for (curr = md5_hashtable[i] ;  curr != NULL;) 
		{
			//If this mesh cannot be freed, go to the next one.
			if (curr->mesh->memStatic)
			{
				prev = curr;
				curr=curr->next;
				continue;
			}

			
			toDelete = curr;   

			//Rearrange the linked list to skip the element we are going to free.
			if (prev == NULL)
			{
				
				md5_hashtable[i] = curr->next;
				curr=md5_hashtable[i];
			}
			else 
			{
				prev->next = curr->next;
				curr=curr->next;
			}

							
			//Free MD5 mesh
			MD5_FreeMesh(toDelete->mesh);
			
			free(toDelete->name);
			toDelete->name = 0;
			
			free(toDelete);
			toDelete=0;
			
			

		}
	}
}

char ENT_LoadEntity(entity_t* entity, const char* filename, uchar usage)
{
	md5_mesh_t* meshCache = NULL;
	
	if (!filename)
		return 0;
	
	//Check mesh cache
	meshCache = ENT_Get(filename);
	if (meshCache)
	{
		entity->model = meshCache ;
		//printf("[ENT_LoadEntity] Cache hit MD5 '%s'.\n",filename);
	}
	else 
	{
		entity->model = (md5_mesh_t*)calloc(1,sizeof(md5_mesh_t)) ;
		
		if (!MD5_LoadMesh(entity->model,filename))
		{
			free(entity->model);
			Log_Printf("Unable to load mesh '%s'.\n",filename);
			return 0;
		}
		
		ENT_Put(entity->model,filename);
	}

	entity->usage = usage;
	
	
	entity->material = 0;
	entity->material =  MATLIB_Get(entity->model->materialName);
	if (!entity->material)
	{
		Log_Printf("[ENT_LoadEntity *****ERROR******] Unknown material: '%s'.\n",entity->model->materialName);
	}
	
	if (usage == ENT_PARTIAL_DRAW)
	{
		entity->numIndices = entity->model->numIndices;
		entity->indices = (ushort*)calloc(entity->model->numIndices, sizeof(ushort)) ;
		memcpy(entity->indices, entity->model->indices, entity->numIndices * sizeof(ushort));
	}
	
	MATLIB_MakeAvailable(entity->material);
	renderer.UpLoadEntityToGPU(entity);
	
	
	
	entity->xAxisRot = 0;
	entity->yAxisRot = 0;
	entity->zAxisRot = 0;
	

	
	return 1;
}



void ENT_GenerateWorldSpaceBBox(entity_t* entity)
{
	//Transform the bbox from modelSpace to WorldSpace.
	
	vec4_t modelSpaceMinPoint ;
	vec4_t modelSpaceMaxPoint;
	vec4_t worldSpaceMinPoint ;
	vec4_t worldSpaceMaxPoint;
	
	
	vectorCopy(entity->model->modelSpacebbox.min,modelSpaceMinPoint);
	modelSpaceMinPoint[3] = 1;
	
	vectorCopy(entity->model->modelSpacebbox.max,modelSpaceMaxPoint);
	modelSpaceMaxPoint[3] = 1;
	
	matrix_transform_vec4t(entity->matrix, modelSpaceMinPoint , worldSpaceMinPoint);
	matrix_transform_vec4t(entity->matrix, modelSpaceMaxPoint , worldSpaceMaxPoint);
	
	
//	printf("bbox min: [%.2f,%.2f,%.2f]\n", worldSpaceMinPoint[0], worldSpaceMinPoint[1], worldSpaceMinPoint[2]);
//	printf("bbox max: [%.2f,%.2f,%.2f]\n", worldSpaceMaxPoint[0], worldSpaceMaxPoint[1], worldSpaceMaxPoint[2]);
	
	//Generate 6 points defining the box limits.
	
	
	
	//Unit cube volume
	//x  y  z
	//0  0  0
	//1  0  0
	//0  0  1
	//1  0  1
	//0  1  0
	//1  1  0
	//0  1  1
	//1  1  1
	
	// 0 0 0
	entity->worldSpacebbox[0][0] = worldSpaceMinPoint[0] ;
	entity->worldSpacebbox[0][1] = worldSpaceMinPoint[1] ;
	entity->worldSpacebbox[0][2] = worldSpaceMinPoint[2] ;
	
	// 1 0 0
	entity->worldSpacebbox[1][0] = worldSpaceMaxPoint[0] ;
	entity->worldSpacebbox[1][1] = worldSpaceMinPoint[1] ;
	entity->worldSpacebbox[1][2] = worldSpaceMinPoint[2] ;

	// 0 0 1
	entity->worldSpacebbox[2][0] = worldSpaceMinPoint[0] ;
	entity->worldSpacebbox[2][1] = worldSpaceMinPoint[1] ;
	entity->worldSpacebbox[2][2] = worldSpaceMaxPoint[2] ;

	//1  0  1
	entity->worldSpacebbox[3][0] = worldSpaceMaxPoint[0] ;
	entity->worldSpacebbox[3][1] = worldSpaceMinPoint[1] ;
	entity->worldSpacebbox[3][2] = worldSpaceMaxPoint[2] ;
		
	//0  1  0
	entity->worldSpacebbox[4][0] = worldSpaceMinPoint[0] ;
	entity->worldSpacebbox[4][1] = worldSpaceMaxPoint[1] ;
	entity->worldSpacebbox[4][2] = worldSpaceMinPoint[2] ;
	
	//1  1  0
	entity->worldSpacebbox[5][0] = worldSpaceMaxPoint[0] ;
	entity->worldSpacebbox[5][1] = worldSpaceMaxPoint[1] ;
	entity->worldSpacebbox[5][2] = worldSpaceMinPoint[2] ;
	
	//0  1  1
	entity->worldSpacebbox[6][0] = worldSpaceMinPoint[0] ;
	entity->worldSpacebbox[6][1] = worldSpaceMaxPoint[1] ;
	entity->worldSpacebbox[6][2] = worldSpaceMaxPoint[2] ;
	
	//1  1  1
	entity->worldSpacebbox[7][0] = worldSpaceMaxPoint[0] ;
	entity->worldSpacebbox[7][1] = worldSpaceMaxPoint[1] ;
	entity->worldSpacebbox[7][2] = worldSpaceMaxPoint[2] ;
}