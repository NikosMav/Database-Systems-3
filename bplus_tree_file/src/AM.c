#include "AM.h"
#include "bf.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

int AM_errno = AME_OK;

#define CALL_BF(call)       \
{                           \
  BF_ErrorCode code = call; \
  if (code != BF_OK) {         \
    BF_PrintError(code);    \
    AM_errno = AM_BF_ERROR;	\
    return AM_BF_ERROR;        \
  }                         \
}

fileInfo* OpenFiles;
scanInfo* ScanFiles;

//ACCESSOR FUNCTIONS
int getAmIndex(int scanDesc) {
  return ScanFiles[scanDesc].AMindex;
}
char* getFileName2 (int ScanDesc) {
  return ScanFiles[ScanDesc].fileName;
}
char* getFileName (int AMDesc) {
  return OpenFiles[AMDesc].fileName;
}
int getFileDesc (int AMDesc) {
  return OpenFiles[AMDesc].fileDesc;
}
char getType1 (int AMDesc) {
  return OpenFiles[AMDesc].type1;
}
char getType2 (int AMDesc) {
  return OpenFiles[AMDesc].type2;
}
int getLength1 (int AMDesc) {
  return OpenFiles[AMDesc].length1;
}
int getLength2 (int AMDesc) {
  return OpenFiles[AMDesc].length2;
}
int getNumOfRecs (int AMDesc) {
  return OpenFiles[AMDesc].numOfRecs;
}
int getNumOfptrs (int AMDesc) {
  return OpenFiles[AMDesc].numOfPtrs;
}

int getNumChildren(char** data){
  return *((int*)(*data+1));
}

int* getIthChildBlockID(char** data, int i, int AMDesc){
  return (int*)(*data+INTERNAL_HEADER_SIZE+i*(getLength1(AMDesc) + sizeof(int)));
}

void* getIthPivot(char** data, int i, int AMDesc){
  return (void*)(getIthChildBlockID(data,i,AMDesc)) + sizeof(int);
}

void* getValue2(char* data,int AMDesc, int* pos){
  return (void*)(data+LEAF_HEADER_SIZE+(getLength1(AMDesc) + getLength2(AMDesc)) * (*pos) + getLength1(AMDesc));
}

//MUTATOR FUNCTIONS
void setFileName2 (char* fileName, int index) {
  ScanFiles[index].fileName = malloc((strlen(fileName) + 1) * sizeof(char));
  strcpy(ScanFiles[index].fileName, fileName);
}
void setFileName (int AMDesc, char* fileName) {
  OpenFiles[AMDesc].fileName = malloc((strlen(fileName) + 1) * sizeof(char));
  strcpy(OpenFiles[AMDesc].fileName, fileName);
}
void setFileDesc (int AMDesc, int fileDesc) {
  OpenFiles[AMDesc].fileDesc = fileDesc;
}
void setType1 (int AMDesc, int type1) {
  OpenFiles[AMDesc].type1 = type1;
}
void setType2 (int AMDesc, int type2) {
  OpenFiles[AMDesc].type2 = type2;
}
void setLength1 (int AMDesc, int length1) {
  OpenFiles[AMDesc].length1 = length1;
}
void setLength2 (int AMDesc, int length2) {
  OpenFiles[AMDesc].length2 = length2;
}
void setNumOfRecs (int AMDesc, int numOfRecs) {
  OpenFiles[AMDesc].numOfRecs = numOfRecs;
}
void setNumOfptrs (int AMDesc, int numOfPtrs) {
  OpenFiles[AMDesc].numOfPtrs = numOfPtrs;
}

//CUSTOM FUNCTIONS
void updateLeafInfo(char** leaf_data){
  int total_entries;
  memcpy(&total_entries, *leaf_data+1, sizeof(int));
  total_entries++;
  memcpy(*leaf_data+1, &total_entries, sizeof(int));
}

int getRootNum(int fileDesc){
  BF_Block* first_block;
  BF_Block_Init(&first_block);
  CALL_BF(BF_GetBlock(fileDesc, 0, first_block));
  char *first_data = BF_Block_GetData(first_block);

  int rootNum;
  memcpy(&rootNum, first_data + 3 + 2*sizeof(int), sizeof(int));

  CALL_BF(BF_UnpinBlock(first_block));
  BF_Block_Destroy(&first_block);

  return rootNum;
}

int updateRootInfo(int fileDesc){
  BF_Block* first_block;
  BF_Block_Init(&first_block);
  CALL_BF(BF_GetBlock(fileDesc, 0, first_block));
  char *first_data = BF_Block_GetData(first_block);

  int rootNum;
  CALL_BF(BF_GetBlockCounter(fileDesc, &rootNum));
  rootNum = rootNum-1;
  memcpy(first_data + 3 + 2*sizeof(int), &rootNum, sizeof(int));

  BF_Block_SetDirty(first_block);
  CALL_BF(BF_UnpinBlock(first_block));
  BF_Block_Destroy(&first_block);

  return AM_errno;
}

/*Prints the key, given that it's type is the one indicated by attrType1*/
void printKey(void *key, char attrType1){
  if (attrType1 == 'c'){
    printf("%s", (char *) key);
  } else if (attrType1 == 'i'){
    printf("%d", *((int *) key));
  } else {
    printf("%f", *((float *) key));
  }
  return;
}

void printInternalBlock(char** data, int AMDesc){
  char attrType1 = getType1(AMDesc);
  int attrLength1 = getLength1(AMDesc);
  int childrenCount = *(int*)(*data+1);
  int i;
  printf("Dict Block with %d children\n", childrenCount);


  for(i = 0; i < childrenCount - 1; i++){
    printf("Child Number : %d ", i);
    printf("Child ID : %d\n", *getIthChildBlockID(data, i, AMDesc));
    printKey(getIthPivot(data, i, AMDesc), attrType1);
    printf("\n");
  }

  printf("Child Number : %d ", i);
  printf("Child ID : %d\n", *getIthChildBlockID(data, i, AMDesc));
  printf("\n");


}

void printLeafBlock(char** data, int AMDesc){/////TODO
  char attrType1 = getType1(AMDesc);
  int attrLength1 = getLength1(AMDesc);
  char attrType2 = getType2(AMDesc);
  int attrLength2 = getLength2(AMDesc);
  int entriesCount = *(int*)(*data+1);
  int i;
  printf("Leaf Block with %d entries\n", entriesCount);


  for(i = 0; i < entriesCount; i++){
    printf("Entry Number : %d ", i);
    //printf("%s - %d\n", *data+LEAF_HEADER_SIZE+i*(getLength1(AMDesc) + getLength2(AMDesc)), *(int*)(*data+LEAF_HEADER_SIZE+i*(getLength1(AMDesc) + getLength2(AMDesc)) + getLength1(AMDesc)));
    printKey((void*)*data+LEAF_HEADER_SIZE+i*(getLength1(AMDesc) + getLength2(AMDesc)), attrType1);
    printf(" - ");
    printKey((void*)*data+LEAF_HEADER_SIZE+i*(getLength1(AMDesc) + getLength2(AMDesc)) + attrLength1, attrType2);
    printf("\n");

  }
}

void writeOnInternal(char** point, void* value1, int AMDesc, int num_children){
  int fileDesc = getFileDesc(AMDesc);
  int i;
  int blocks_num;
  BF_GetBlockCounter(fileDesc, &blocks_num);
  blocks_num--;

  for(i = 0; i < num_children - 1; i++){

    if(getType1(AMDesc) == 'c'){
      char* temp_value1;
      temp_value1 = malloc(getLength1(AMDesc));
      memcpy(temp_value1, (char*)value1, getLength1(AMDesc));

      if(strcmp(temp_value1, *point+ INTERNAL_HEADER_SIZE + i*(getLength1(AMDesc)+sizeof(int)) + sizeof(int)) >= 0){ //check current key with new key
        continue;
      }else{
        void* temp;
        temp = malloc((num_children-i-1) * (getLength1(AMDesc) + sizeof(int)));
        memcpy(temp, *point+ INTERNAL_HEADER_SIZE + i*(getLength1(AMDesc)+sizeof(int)) + sizeof(int), (num_children-i-1) * (getLength1(AMDesc) + sizeof(int)));
        //getting last block aka new block
        memcpy(*point + INTERNAL_HEADER_SIZE + i*(getLength1(AMDesc)+sizeof(int)) + sizeof(int), value1, getLength1(AMDesc));
        memcpy(*point + INTERNAL_HEADER_SIZE + i*(getLength1(AMDesc)+sizeof(int)) + sizeof(int) + getLength1(AMDesc), &blocks_num, sizeof(int));
        memcpy(*point + INTERNAL_HEADER_SIZE + (i+1)*(getLength1(AMDesc) + sizeof(int)) + sizeof(int), temp, (num_children-i-1) * (getLength1(AMDesc) + sizeof(int)));
        free(temp);
        break;
      }

    }else if (getType1(AMDesc) == 'i') {

      int temp_value1 = *(int*)value1;
      int blocks_num;
      BF_GetBlockCounter(fileDesc, &blocks_num);
      blocks_num--;

      if(temp_value1 >= *(int*)(*point+ INTERNAL_HEADER_SIZE + i*(getLength1(AMDesc)+sizeof(int)) + sizeof(int))){
        continue;
      }else{
        void* temp;
        temp = malloc((num_children-i-1) * (getLength1(AMDesc) + sizeof(int)));//TODO FROM HERE
        memcpy(temp, *point+ INTERNAL_HEADER_SIZE + i*(getLength1(AMDesc)+sizeof(int)) + sizeof(int), (num_children-i-1) * (getLength1(AMDesc) + sizeof(int)));
        //getting last block aka new block
        memcpy(*point + INTERNAL_HEADER_SIZE + i*(getLength1(AMDesc)+sizeof(int)) + sizeof(int), value1, getLength1(AMDesc));
        memcpy(*point + INTERNAL_HEADER_SIZE + i*(getLength1(AMDesc)+sizeof(int)) + sizeof(int) + getLength1(AMDesc), &blocks_num, sizeof(int));
        memcpy(*point + INTERNAL_HEADER_SIZE + (i+1)*(getLength1(AMDesc) + sizeof(int)) + sizeof(int), temp, (num_children-i-1) * (getLength1(AMDesc) + sizeof(int)));
        free(temp);
        break;
      }

    }else if(getType1(AMDesc) == 'f'){
      float temp_value1 = *(float*)value1;
      int blocks_num;
      BF_GetBlockCounter(fileDesc, &blocks_num);
      blocks_num--;

      if(temp_value1 >= *(float*)(*point+ INTERNAL_HEADER_SIZE + i*(getLength1(AMDesc)+sizeof(int)) + sizeof(int))){
        continue;
      }else{
        void* temp;
        temp = malloc((num_children-i-1) * (getLength1(AMDesc) + sizeof(int)));
        memcpy(temp, *point+ INTERNAL_HEADER_SIZE + i*(getLength1(AMDesc)+sizeof(int)) + sizeof(int), (num_children-i-1) * (getLength1(AMDesc) + sizeof(int)));
        //getting last block aka new block
        memcpy(*point + INTERNAL_HEADER_SIZE + i*(getLength1(AMDesc)+sizeof(int)) + sizeof(int), value1, getLength1(AMDesc));
        memcpy(*point + INTERNAL_HEADER_SIZE + i*(getLength1(AMDesc)+sizeof(int)) + sizeof(int) + getLength1(AMDesc), &blocks_num, sizeof(int));
        memcpy(*point + INTERNAL_HEADER_SIZE + (i+1)*(getLength1(AMDesc) + sizeof(int)) + sizeof(int), temp, (num_children-i-1) * (getLength1(AMDesc) + sizeof(int)));
        free(temp);
        break;
      }

    }
  }
  if(i == num_children-1){
    memcpy(*point + INTERNAL_HEADER_SIZE + i*(getLength1(AMDesc)+sizeof(int)) + sizeof(int), value1, getLength1(AMDesc));
    memcpy(*point + INTERNAL_HEADER_SIZE + i*(getLength1(AMDesc)+sizeof(int)) + sizeof(int) + getLength1(AMDesc), &blocks_num, sizeof(int));
  }
}

void writeOnLeaf(char** point, void* value1, void* value2, int AMDesc,int total_entries){

  int i;
  for(i = 0; i <= total_entries; i++){

    if(getType1(AMDesc) == 'c'){
      char* temp_value1;
      temp_value1 = malloc(getLength1(AMDesc));
      strcpy(temp_value1, (char*)value1);

      if(i < total_entries){
        if(strcmp(temp_value1, *point+ LEAF_HEADER_SIZE + i*(getLength1(AMDesc)+getLength2(AMDesc))) >= 0){
          free(temp_value1);
          continue;
        }else{
          void* temp;
          temp = malloc((total_entries-i) * (getLength1(AMDesc) + getLength2(AMDesc)));
          memcpy(temp, *point+ LEAF_HEADER_SIZE + i*(getLength1(AMDesc)+getLength2(AMDesc)), (total_entries-i) * (getLength1(AMDesc) + getLength2(AMDesc)));
          memcpy(*point+ LEAF_HEADER_SIZE + i*(getLength1(AMDesc)+getLength2(AMDesc)), value1, getLength1(AMDesc));
          memcpy(*point+ LEAF_HEADER_SIZE + i*(getLength1(AMDesc)+getLength2(AMDesc))+ getLength1(AMDesc), value2, getLength2(AMDesc));
          memcpy(*point+ LEAF_HEADER_SIZE + i*(getLength1(AMDesc)+getLength2(AMDesc))+ getLength1(AMDesc) + getLength2(AMDesc), temp, (total_entries-i) * (getLength1(AMDesc) + getLength2(AMDesc)));
          free(temp);
          break;
        }
      }

      memcpy(*point+ LEAF_HEADER_SIZE + i*(getLength1(AMDesc)+getLength2(AMDesc)), value1, getLength1(AMDesc)); // write value1
      memcpy(*point + LEAF_HEADER_SIZE + i*(getLength1(AMDesc)+getLength2(AMDesc))+ getLength1(AMDesc), value2, getLength2(AMDesc)); //write value2

      break;

    }else if (getType1(AMDesc) == 'i') {

      int temp_value1 = *(int*)value1;
      if(i < total_entries){
        if(temp_value1 >= *(int*)(*point+ LEAF_HEADER_SIZE + i*(getLength1(AMDesc)+getLength2(AMDesc)))){
          continue;
        }else{
          void* temp;
          temp = malloc((total_entries-i) * (getLength1(AMDesc) + getLength2(AMDesc)));
          memcpy(temp, *point+ LEAF_HEADER_SIZE + i*(getLength1(AMDesc)+getLength2(AMDesc)), (total_entries-i) * (getLength1(AMDesc) + getLength2(AMDesc)));
          memcpy(*point+ LEAF_HEADER_SIZE + i*(getLength1(AMDesc)+getLength2(AMDesc)), value1, getLength1(AMDesc));
          memcpy(*point+ LEAF_HEADER_SIZE + i*(getLength1(AMDesc)+getLength2(AMDesc))+ getLength1(AMDesc), value2, getLength2(AMDesc));
          memcpy(*point+ LEAF_HEADER_SIZE + i*(getLength1(AMDesc)+getLength2(AMDesc))+ getLength1(AMDesc) + getLength2(AMDesc), temp, (total_entries-i) * (getLength1(AMDesc) + getLength2(AMDesc)));
          free(temp);
          break;
        }
      }


      memcpy(*point+ LEAF_HEADER_SIZE + i*(getLength1(AMDesc)+getLength2(AMDesc)), value1, getLength1(AMDesc)); // write value1
      memcpy(*point + LEAF_HEADER_SIZE + i*(getLength1(AMDesc)+getLength2(AMDesc))+ getLength1(AMDesc), value2, getLength2(AMDesc)); //write value2

      break;

    }else if(getType1(AMDesc) == 'f'){
      float temp_value1 = *(float*)value1;
      if( i < total_entries){
        if(temp_value1 >= *(float*)(*point+ LEAF_HEADER_SIZE + i*(getLength1(AMDesc)+getLength2(AMDesc)))){
          continue;
        }else{
          void* temp;
          temp = malloc((total_entries-i) * (getLength1(AMDesc) + getLength2(AMDesc)));
          memcpy(temp, *point+ LEAF_HEADER_SIZE + i*(getLength1(AMDesc)+getLength2(AMDesc)), (total_entries-i) * (getLength1(AMDesc) + getLength2(AMDesc)));
          memcpy(*point+ LEAF_HEADER_SIZE + i*(getLength1(AMDesc)+getLength2(AMDesc)), value1, getLength1(AMDesc));
          memcpy(*point+ LEAF_HEADER_SIZE + i*(getLength1(AMDesc)+getLength2(AMDesc))+ getLength1(AMDesc), value2, getLength2(AMDesc));
          memcpy(*point+ LEAF_HEADER_SIZE + i*(getLength1(AMDesc)+getLength2(AMDesc))+ getLength1(AMDesc), value2, getLength2(AMDesc));
          memcpy(*point+ LEAF_HEADER_SIZE + i*(getLength1(AMDesc)+getLength2(AMDesc))+ getLength1(AMDesc) + getLength2(AMDesc), temp, (total_entries-i) * (getLength1(AMDesc) + getLength2(AMDesc)));
          free(temp);
          break;
        }
      }
      memcpy(*point+ LEAF_HEADER_SIZE + i*(getLength1(AMDesc)+getLength2(AMDesc)), value1, getLength1(AMDesc)); // write value1
      memcpy(*point + LEAF_HEADER_SIZE + i*(getLength1(AMDesc)+getLength2(AMDesc))+ getLength1(AMDesc), value2, getLength2(AMDesc)); //write value2

      break;
    }
  }
}

int InsertEntryToLeaf(char** leaf_data, int AMDesc, void* value1, void* value2){
  int total_entries, child;
  memcpy(&total_entries, *leaf_data+1, sizeof(int));

  int max_entries = getNumOfRecs(AMDesc);

  if(total_entries < max_entries){
    writeOnLeaf(leaf_data, value1, value2, AMDesc, total_entries);
    updateLeafInfo(leaf_data);
    return 0;
  }else{
    return -1;
  }
}

int getSuitableChild(char** data, int AMDesc, void* value1){
  int num_children, wanted_BlockNum,i;

  memcpy(&num_children, *data + 1, sizeof(int)); // children of block
  for(i = 0; i < num_children - 1; i++){// getting suitable child/////////??????????????

    if(getType1(AMDesc) == 'c'){//////////////////////// VALUE1 == CHAR
      char* temp_value1 = malloc(getLength1(AMDesc));
      memcpy(temp_value1, value1, getLength1(AMDesc));
      char* curr_value = malloc(getLength1(AMDesc));
      memcpy(curr_value,*data + INTERNAL_HEADER_SIZE + i*(sizeof(int) + getLength1(AMDesc)) + sizeof(int), getLength1(AMDesc));

      if(strcmp(temp_value1,curr_value) < 0){
        wanted_BlockNum = *getIthChildBlockID(data, i, AMDesc);
        free(temp_value1);
        free(curr_value);
        break;
      }
      free(temp_value1);
      free(curr_value);

    }else if(getType1(AMDesc) == 'i'){
      int temp_value1 = *(int*)value1;
      int curr_value = *(int*)(*data+INTERNAL_HEADER_SIZE+i*(sizeof(int) + getLength1(AMDesc)) + sizeof(int));
      if(temp_value1 < curr_value){
        wanted_BlockNum = *getIthChildBlockID(data, i, AMDesc);
        break;
      }

    }else if(getType1(AMDesc) == 'f'){
      float temp_value1 = *(float*)value1;
      float curr_value = *(float*)(*data+INTERNAL_HEADER_SIZE+i*(sizeof(int) + getLength1(AMDesc)) + sizeof(int));

      if(temp_value1 < curr_value){
        wanted_BlockNum = *getIthChildBlockID(data, i, AMDesc);
        break;
      }
    }
  }

  if(i == num_children-1){
    wanted_BlockNum = *getIthChildBlockID(data, i, AMDesc);

  }

  return wanted_BlockNum;
}

void chooseCorrectBlock(char** data,char** new_data, int AMDesc, void** mid_value){
  int num_children = getNumOfptrs(AMDesc);
  int num_entries = num_children - 1;
  char* temp = malloc(INTERNAL_HEADER_SIZE + (num_entries + 1) * (getLength1(AMDesc) + sizeof(int)) + sizeof(int));
  memcpy(temp, *data, INTERNAL_HEADER_SIZE + num_entries * (getLength1(AMDesc) + sizeof(int)) + sizeof(int));
  writeOnInternal(&(temp), *mid_value, AMDesc, num_children+1);

  int ceil_num_children = (int)ceil(((float)num_children + 1.0)/2.0)+1;
  int floor_num_children = (int)floor(((float)num_children + 1.0)/2.0)+1;
  //printf("CEIL = %d || floor = %d\n", ceil_numOfPtrs,floor_numOfPtrs);
  memcpy(*data + INTERNAL_HEADER_SIZE, temp + INTERNAL_HEADER_SIZE, sizeof(int) + (getLength1(AMDesc)+sizeof(int))* floor_num_children);
  memcpy(*new_data + INTERNAL_HEADER_SIZE, temp + INTERNAL_HEADER_SIZE + (floor_num_children)*(getLength1(AMDesc) + sizeof(int)), sizeof(int) + (getLength1(AMDesc)+sizeof(int))* ceil_num_children);

  memcpy(*new_data+1, &ceil_num_children, sizeof(int));/////////////////////////////////////////
  //MOVING DATA
  memcpy(*data+1, &floor_num_children, sizeof(int));
  //insert ceil_new_entries on new block


  //set mid_value as the first value of right block
  memcpy(*mid_value, new_data+INTERNAL_HEADER_SIZE + sizeof(int), getLength1(AMDesc));


  free(temp);

  //SHIFTING DATA ONE PLACE LEFT
  temp = malloc(sizeof(int) + (getLength1(AMDesc) + sizeof(int))*(ceil_num_children-1));
  memcpy(temp, *new_data+INTERNAL_HEADER_SIZE + (getLength1(AMDesc) + sizeof(int)), sizeof(int) + (getLength1(AMDesc) + sizeof(int))*(ceil_num_children-1));
  memcpy(*new_data + INTERNAL_HEADER_SIZE, temp, sizeof(int) + (getLength1(AMDesc) + sizeof(int))*(ceil_num_children-1));
  ceil_num_children--;
  memcpy(*new_data + 1, &ceil_num_children, sizeof(int));

  free(temp);


}

int insertOrSplit(char** data, int AMDesc, int depth, void* value1, void* value2, void** mid_value){
  int succesful_insertion;
  int fileDesc = getFileDesc(AMDesc);

  // BASE CASE : ON LEAF NODE
  if(*(*data) == 'L'){
    // printf("BEFORE INSERTION:\n");
    // printLeafBlock(data,AMDesc);
    // printf("ABOUT TO INSERT:\n");
    // printKey(value1, 'c');
    // printf("\n");
    succesful_insertion = InsertEntryToLeaf(data, AMDesc, value1, value2);
    // printf("AFTER INSERTION\n");
    // printLeafBlock(data, AMDesc);
    if(succesful_insertion == -1){
      char* new_data;

      // NEW LEAF NODE
      BF_Block* new_block;
      BF_Block_Init(&new_block);
      CALL_BF(BF_AllocateBlock(fileDesc,new_block));
      new_data = BF_Block_GetData(new_block);

      (*new_data) = 'L';
      int current_entries = 0;
      memcpy(new_data + 1, &current_entries, sizeof(int));

      int num_entries;
      memcpy(&num_entries, *data+1, sizeof(int));
      //printf("num_entries = %d\n",num_entries );
      int ceil_new_entries = (int)ceil(((float)num_entries + 1.0)/2.0);
      int floor_new_entries = (int)floor(((float)num_entries + 1.0)/2.0);
      //printf("CEIL = %d || floor = %d\n", ceil_new_entries, floor_new_entries);

      ceil_new_entries;
      memcpy(new_data+1, &ceil_new_entries, sizeof(int));
      //MOVING DATA
      char* temp = malloc((LEAF_HEADER_SIZE + getLength1(AMDesc)+getLength2(AMDesc))* (num_entries + 1));
      memcpy(temp, *data,LEAF_HEADER_SIZE + (getLength1(AMDesc) + getLength2(AMDesc)) * (num_entries));

      writeOnLeaf(&temp, value1, value2, AMDesc, num_entries);
      updateLeafInfo(&temp);
      // printf("MADE TEMp BLOCK\n");
      // printLeafBlock(&temp, AMDesc);

      memcpy(*data+1, &floor_new_entries, sizeof(int));
      //insert ceil_new_entries on new block
      memcpy(*data+LEAF_HEADER_SIZE, temp+LEAF_HEADER_SIZE, (getLength1(AMDesc) + getLength2(AMDesc)) * floor_new_entries);
      memcpy(new_data+LEAF_HEADER_SIZE, temp+LEAF_HEADER_SIZE+ (floor_new_entries)*(getLength1(AMDesc) + getLength2(AMDesc)), (getLength1(AMDesc) + getLength2(AMDesc)) * ceil_new_entries);

      //set mid_value as the first value of right block
      memcpy(*mid_value, new_data+LEAF_HEADER_SIZE, getLength1(AMDesc));

      // showing right node to left node
      memcpy(new_data+1+sizeof(int), *data + 1 + sizeof(int), sizeof(int));
      int new_leaf_num;
      CALL_BF(BF_GetBlockCounter(fileDesc, &new_leaf_num));
      new_leaf_num--;
      memcpy(*data+1+sizeof(int), &new_leaf_num, sizeof(int));

      // printf("BROKE INTO:\n");
      // printLeafBlock(data, AMDesc);
      // printLeafBlock(&new_data, AMDesc);

      BF_Block_SetDirty(new_block);
      CALL_BF(BF_UnpinBlock(new_block));
      BF_Block_Destroy(&new_block);


      if(depth == 0){ // if leaf is root

        char* root_data;
        int left_child = getRootNum(fileDesc);
        int right_child;
        BF_Block* new_root;
        // Internal block creation
        CALL_BF(BF_GetBlockCounter(fileDesc, &right_child));
        right_child --;
        BF_Block_Init(&new_root);
        CALL_BF(BF_AllocateBlock(fileDesc,new_root));
        root_data = BF_Block_GetData(new_root);
        *root_data = 'I';
        int current_children = 2;
        memcpy(root_data + 1, &current_children, sizeof(int));

        memcpy(root_data+INTERNAL_HEADER_SIZE, &left_child, sizeof(int)); // inserting left child
        memcpy(root_data+INTERNAL_HEADER_SIZE+sizeof(int), *mid_value, getLength1(AMDesc)); // inserting key
        memcpy(root_data+INTERNAL_HEADER_SIZE+sizeof(int) + getLength1(AMDesc), &right_child, sizeof(int)); //inserting right child

        updateRootInfo(fileDesc);

        BF_Block_SetDirty(new_root);
        CALL_BF(BF_UnpinBlock(new_root));
        BF_Block_Destroy(&new_root);

      }

      free(temp);
      return -1;
    }

    return 0;

  }else if(*(*data) == 'I'){// if in internal block
    BF_Block* child_block;
    BF_Block_Init(&child_block);
    int suitable_child;

    printInternalBlock(data,AMDesc);
    //getting suitable child
    suitable_child = getSuitableChild(data, AMDesc, value1);

    CALL_BF(BF_GetBlock(fileDesc, suitable_child, child_block));
    char* child_data = BF_Block_GetData(child_block);

    int value = insertOrSplit(&child_data, AMDesc, depth+1, value1, value2, mid_value); //recursion

    BF_Block_SetDirty(child_block);
    CALL_BF(BF_UnpinBlock(child_block));
    BF_Block_Destroy(&child_block);

    if(value == 0){
      return 0;
    }

    if(getNumOfptrs(AMDesc) == getNumChildren(data)){
      char* new_data;
      void* temp;

      // NEW INTERNAL NODE
      BF_Block* new_internal_block;
      BF_Block_Init(&new_internal_block);
      CALL_BF(BF_AllocateBlock(fileDesc,new_internal_block));
      new_data = BF_Block_GetData(new_internal_block);

      (*new_data) = 'I';
      int current_children = 0;
      memcpy(new_data + 1, &current_children, sizeof(int));

      chooseCorrectBlock(data, &new_data, AMDesc, mid_value);

      BF_Block_SetDirty(new_internal_block);
      CALL_BF(BF_UnpinBlock(new_internal_block));
      BF_Block_Destroy(&new_internal_block);

      if(depth == 0){
        char* root_data;
        int left_child = getRootNum(fileDesc);
        int right_child;
        // Internal block creation
        CALL_BF(BF_GetBlockCounter(fileDesc, &right_child));
        right_child = right_child-1;
        BF_Block* new_root;
        BF_Block_Init(&new_root);
        CALL_BF(BF_AllocateBlock(fileDesc,new_root));
        root_data = BF_Block_GetData(new_root);
        *root_data = 'I';
        int current_children = 2;
        memcpy((root_data)+1, &current_children, sizeof(int));

        memcpy(root_data+INTERNAL_HEADER_SIZE, &left_child, sizeof(int)); // inserting left child
        memcpy(root_data+INTERNAL_HEADER_SIZE+sizeof(int), *mid_value, getLength1(AMDesc)); // inserting key
        memcpy(root_data+INTERNAL_HEADER_SIZE+sizeof(int) + getLength1(AMDesc), &right_child, sizeof(int)); //inserting right child
        updateRootInfo(fileDesc);

        BF_Block_SetDirty(new_root);
        CALL_BF(BF_UnpinBlock(new_root));
        BF_Block_Destroy(&new_root);
      }
      return -1;

    }else{

      writeOnInternal(data,*mid_value,AMDesc,getNumChildren(data));
      int children;
      memcpy(&children, *data+1, sizeof(int));
      children++;
      memcpy(*data+1, &children, sizeof(int));
      return 0;
    }
  }
}

int Comparison(int AMDesc, void* key1, void* key2){
  if(getType1(AMDesc) == 'c'){
    char* temp1 = malloc(getLength1(AMDesc));
    char* temp2 = malloc(getLength1(AMDesc));
    memcpy(temp1, key1, getLength1(AMDesc));
    memcpy(temp2, key2, getLength1(AMDesc));
    return strcmp(temp1, temp2);
  }else if(getType1(AMDesc) == 'i'){
    if(*(int*)key1 > *(int*)key2){
      return 1;
    }else if(*(int*)key1 == *(int*)key2){
      return 0;
    }else{
      return -1;
    }
  }else{
    if(*(float*)key1 > *(float*)key2){
      return 1;
    }else if(*(float*)key1 == *(float*)key2){
      return 0;
    }else{
      return -1;
    }
  }
}

int navigate(char* data, int AMDesc, int op, void* value){
  if(*data == 'L'){
    return 0;
  }

  int fileDesc = getFileDesc(AMDesc);
  int child;
  int suitable_child;
  BF_Block* child_block;
  char* child_data;

  BF_Block_Init(&child_block);

  if(op == NOT_EQUAL || op == LESS_THAN || op == LESS_THAN_OR_EQUAL){
    memcpy(&child, data+INTERNAL_HEADER_SIZE, sizeof(int));
  }else{
    child = getSuitableChild(&data, AMDesc, value);
  }

  CALL_BF(BF_GetBlock(fileDesc, child, child_block));
  child_data = BF_Block_GetData(child_block);

  CALL_BF(BF_UnpinBlock(child_block));
  BF_Block_Destroy(&child_block);

  suitable_child = navigate(child_data, AMDesc, op, value);

  if(suitable_child == 0){
    return child;
  }
  return suitable_child;
}

void AM_Init() {
  int i;

  BF_Init(LRU);
  //Creating the Array for Opened Files
  OpenFiles = malloc(MAXOPENFILES * sizeof(fileInfo));
  for (i = 0; i < MAXOPENFILES; i++) {
    OpenFiles[i].fileName = NULL;
    OpenFiles[i].fileDesc = -1;
    OpenFiles[i].type1 = -1;
    OpenFiles[i].type2 = -1;
    OpenFiles[i].length1 = -1;
    OpenFiles[i].length2 = -1;
    OpenFiles[i].numOfRecs = -1;
    OpenFiles[i].numOfPtrs = -1;
  }

  //Creating the Array for Scanned Files
  ScanFiles = malloc(MAXSCANS * sizeof(scanInfo));
  for (i = 0; i < MAXSCANS; i++) {
    ScanFiles[i].fileName = NULL;
    ScanFiles[i].AMindex = -1;
    ScanFiles[i].op = -1;
    ScanFiles[i].comparisonKey = NULL;
    ScanFiles[i].currentBlock = -1;
    ScanFiles[i].entryPos = -1;
  }

  return;
}


int AM_CreateIndex(char *fileName, char attrType1, int attrLength1, char attrType2, int attrLength2) {

  if (attrType1 == 'c') {
    if (!(attrLength1 >= 1 && attrLength1 <= 255)) {
      AM_errno = AM_ATTR_ERROR;
      return AM_ATTR_ERROR;
    }
  } else if (attrType1 == 'i') {
    if (!(attrLength1 == 4)) {
      AM_errno = AM_ATTR_ERROR;
      return AM_ATTR_ERROR;
    }
  } else if (attrType1 == 'f') {
    if (!(attrLength1 == 4)) {
      AM_errno = AM_ATTR_ERROR;
      return AM_ATTR_ERROR;
    }
  } else if (attrType2 == 'c') {
    if (!(attrLength2 >= 1 && attrLength2 <= 255)) {
      AM_errno = AM_ATTR_ERROR;
      return AM_ATTR_ERROR;
    }
  } else if (attrType2 == 'i') {
    if (!(attrLength2 == 4)) {
      AM_errno = AM_ATTR_ERROR;
      return AM_ATTR_ERROR;
    }
  } else if (attrType2 == 'f') {
    if (!(attrLength2 == 4)) {
      AM_errno = AM_ATTR_ERROR;
      return AM_ATTR_ERROR;
    }
  }

  int fileDesc;
  BF_Block* firstBlock;
  BF_Block* rootBlock;

  CALL_BF(BF_CreateFile(fileName));
  CALL_BF(BF_OpenFile(fileName, &fileDesc));

  BF_Block_Init(&firstBlock);
  CALL_BF(BF_AllocateBlock(fileDesc,firstBlock));

  char* data = BF_Block_GetData(firstBlock);
  *data = 'B';       //Defining the B+Tree File
  memcpy(data + 1, &attrType1, sizeof(char));
  memcpy(data + 2, &attrType2, sizeof(char));
  memcpy(data + 3, &attrLength1, sizeof(int));
  memcpy(data + 3 + sizeof(int), &attrLength2, sizeof(int));

  //Defining Root Number
  int rootNum = 1;
  memcpy(data + 3 + 2*sizeof(int), &rootNum, sizeof(int));

  BF_Block_SetDirty(firstBlock);
  CALL_BF(BF_UnpinBlock(firstBlock));
  BF_Block_Destroy(&firstBlock);

  //Creating the Root Block of the file
  BF_Block_Init(&rootBlock);
  CALL_BF(BF_AllocateBlock(fileDesc,rootBlock));

  //Defining Root Block Info
  data = BF_Block_GetData(rootBlock);
  *data = 'L';
  int curNumOfRecs = 0;
  int nextBlock = -1;

  memcpy(data + 1, &curNumOfRecs, sizeof(int));
  memcpy(data + 1 + sizeof(int), &nextBlock, sizeof(int));

  BF_Block_SetDirty(rootBlock);
  CALL_BF(BF_UnpinBlock(rootBlock));
  BF_Block_Destroy(&rootBlock);

  //CALL_BF(BF_CloseFile(fileDesc));
  return AME_OK;
}


int AM_DestroyIndex(char *fileName) {
  int i;

  for(i = 0; i < MAXOPENFILES; i++){
    if(OpenFiles[i].fileName != NULL){
      if(strcmp(OpenFiles[i].fileName, fileName) == 0){
        return AM_FAILED_DESTROY_INDEX;
      }
    }
  }

  remove(fileName);


  return AME_OK;
}


int AM_OpenIndex (char *fileName) {

  int fileDesc;
  BF_Block *block;
  int i;
  for(i = 0; i < MAXOPENFILES; i++){
    if(getFileDesc(i) == -1){
      break;
    }
  }

  //Possible Errors
  if(getFileName(i) != NULL) {
    if (!(strcmp(fileName, getFileName(i)))) {
      AM_errno = AM_AL_OPENED;
      return AM_AL_OPENED;
    }
  }
  if(i == MAXOPENFILES){
    AM_errno = AM_FULLMOP;
    return AM_FULLMOP;
  }

  BF_Block_Init(&block);
  CALL_BF(BF_OpenFile(fileName, &fileDesc));
  CALL_BF(BF_GetBlock(fileDesc, 0, block));

  char* data = BF_Block_GetData(block);

  if(*data != 'B'){
    CALL_BF(BF_UnpinBlock(block));
    BF_Block_Destroy(&block);
    AM_errno = AM_WRONGFILE;
    return AM_WRONGFILE;
  }

  setFileDesc(i, fileDesc);
  char type1 = *(data + 1);
  setType1(i, type1);
  char type2 = *(data + 2);
  setType2(i, type2);
  int length1;
  memcpy(&length1, data + 3, sizeof(int));
  setLength1(i, length1);
  int length2;
  memcpy(&length2, data + 3 + sizeof(int), sizeof(int));
  setLength2(i,length2);
  int numOfRecs = (BF_BLOCK_SIZE - LEAF_HEADER_SIZE) / (OpenFiles[i].length1 + OpenFiles[i].length2);
  setNumOfRecs(i, numOfRecs);
  int numOfPtrs = 1+(BF_BLOCK_SIZE - INTERNAL_HEADER_SIZE - sizeof(int)) / (sizeof(int) + OpenFiles[i].length1);
  setNumOfptrs(i, numOfPtrs);
  setFileName(i,fileName);

  CALL_BF(BF_UnpinBlock(block));
  BF_Block_Destroy(&block);

  return i;
}


int AM_CloseIndex (int AMDesc) {
  int i;
  int fileDesc = getFileDesc(AMDesc);

  for(i = 0; i < MAXSCANS; i++) {
    if(AMDesc == getAmIndex(i)) {
      if(getAmIndex(i) != -1) {
        return AM_FAILED_CLOSE_INDEX;
      }
    }
  }

  CALL_BF(BF_CloseFile(fileDesc));

  OpenFiles[AMDesc].fileName = NULL;
  OpenFiles[AMDesc].fileDesc = -1;
  OpenFiles[AMDesc].type1 = -1;
  OpenFiles[AMDesc].type2 = -1;
  OpenFiles[AMDesc].length1 = -1;
  OpenFiles[AMDesc].length2 = -1;
  OpenFiles[AMDesc].numOfRecs = -1;
  OpenFiles[AMDesc].numOfPtrs = -1;
  return AME_OK;
}



int AM_InsertEntry(int AMDesc, void *value1, void *value2) {
  BF_Block* block;
  BF_Block* root_block;
  int blocks_num;
  int fileDesc = getFileDesc(AMDesc);

  BF_Block_Init(&block);
  CALL_BF(BF_GetBlock(fileDesc, 0, block));
  char* data = BF_Block_GetData(block);

  int rootNum;
  memcpy(&rootNum, data + 3 + 2*sizeof(int), sizeof(int));

  CALL_BF(BF_UnpinBlock(block));
  BF_Block_Destroy(&block);

  BF_Block_Init(&root_block);
  CALL_BF(BF_GetBlock(fileDesc,rootNum,root_block));
  char* root_data = BF_Block_GetData(root_block);

  CALL_BF(BF_GetBlockCounter(fileDesc, &blocks_num));

  void* mid_value = malloc(getLength1(AMDesc));

  int error = insertOrSplit(&root_data, AMDesc, 0, value1, value2, &mid_value);

  BF_Block_SetDirty(root_block);
  CALL_BF(BF_UnpinBlock(root_block));
  BF_Block_Destroy(&root_block);

  if(error != 0 && error != -1){
    return AM_FAILED_INSERT;
  }

  return AME_OK;
}


int AM_OpenIndexScan(int AMDesc, int op, void *value) {
  int fileDesc = getFileDesc(AMDesc);
  char* fileName = getFileName(AMDesc);
  BF_Block* root_block;
  int rootNum = getRootNum(fileDesc);
  char* root_data;
  int i;
  for(i = 0; i < MAXSCANS; i++){
    if(ScanFiles[i].AMindex == -1){
      setFileName2(fileName, i);
      ScanFiles[i].AMindex = AMDesc;
      ScanFiles[i].op = op;
      ScanFiles[i].comparisonKey = malloc(getLength1(AMDesc));
      memcpy(ScanFiles[i].comparisonKey, value, getLength1(AMDesc));
      ScanFiles[i].entryPos = 0;
      break;
    }
  }
  if(i == MAXSCANS){
    return AM_MAX_SCANS_EXCEEDED;
  }


  BF_Block_Init(&root_block);
  CALL_BF(BF_GetBlock(fileDesc, rootNum, root_block));
  root_data = BF_Block_GetData(root_block);

  ScanFiles[i].currentBlock = navigate(root_data, AMDesc, op, value);

  CALL_BF(BF_UnpinBlock(root_block));
  BF_Block_Destroy(&root_block);

  return i;
  //return AME_OK;
}

void MoveForward(int scanDesc, int AMDesc){
  int op = ScanFiles[scanDesc].op;
  void* value = ScanFiles[scanDesc].comparisonKey;
  int block_num = ScanFiles[scanDesc].currentBlock;
  int pos = ScanFiles[scanDesc].entryPos;
  int fileDesc = getFileDesc(AMDesc);
  char* data;
  BF_Block* block;

  BF_Block_Init(&block);
  BF_GetBlock(fileDesc, block_num, block);

  data = BF_Block_GetData(block);

  BF_UnpinBlock(block);
  BF_Block_Destroy(&block);

  pos++;

  if(pos == *(int*)(data+1)){
    pos = 0;
    memcpy(&block_num, data+1+sizeof(int), sizeof(int));
    ScanFiles[scanDesc].currentBlock = block_num;
  }

  ScanFiles[scanDesc].entryPos = pos;
}

void *AM_FindNextEntry(int scanDesc) {
  int AMDesc = ScanFiles[scanDesc].AMindex;
  int op = ScanFiles[scanDesc].op;
  void* value = ScanFiles[scanDesc].comparisonKey;
  int block_num = ScanFiles[scanDesc].currentBlock;
  int pos = ScanFiles[scanDesc].entryPos;

  int fileDesc = getFileDesc(AMDesc);
  char* data;
  BF_Block* block;
  void* value2;

  if(block_num == -1){
    AM_errno = AME_EOF;
    return NULL;
  }

  BF_Block_Init(&block);
  BF_GetBlock(fileDesc, block_num, block);

  data = BF_Block_GetData(block);

  BF_UnpinBlock(block);
  BF_Block_Destroy(&block);

  value2 = getValue2(data, AMDesc, &pos);
  //check with op
  if(op == EQUAL){
    for(pos; pos < *(int*)(data+1); pos++){
      value2 = getValue2(data, AMDesc, &pos);

      if(Comparison(AMDesc, value, (void*)(data+LEAF_HEADER_SIZE+(getLength1(AMDesc) + getLength2(AMDesc)) * pos)) == 0){
        return value2;
      }
    }
  }else if(op == NOT_EQUAL){
    if(Comparison(AMDesc, value, (void*)(data+LEAF_HEADER_SIZE+(getLength1(AMDesc) + getLength2(AMDesc)) * pos)) != 0){

      MoveForward(scanDesc, AMDesc);
      return value2;
    }else{
      MoveForward(scanDesc, AMDesc);
      value2 = AM_FindNextEntry(scanDesc);
      return value2;
    }



  }else if(op == LESS_THAN_OR_EQUAL){

    if(Comparison(AMDesc, value, (void*)(data+LEAF_HEADER_SIZE+(getLength1(AMDesc) + getLength2(AMDesc)) * pos)) >= 0){
      MoveForward(scanDesc, AMDesc);
      return value2;
    }
  }else if(op == LESS_THAN){
      if(Comparison(AMDesc, value, (void*)(data+LEAF_HEADER_SIZE+(getLength1(AMDesc) + getLength2(AMDesc)) * pos)) > 0){
        MoveForward(scanDesc, AMDesc);
        return value2;
    }
  }else if(op == GREATER_THAN){
      if(Comparison(AMDesc, value, (void*)(data+LEAF_HEADER_SIZE+(getLength1(AMDesc) + getLength2(AMDesc)) * pos)) < 0){
        MoveForward(scanDesc, AMDesc);
        return value2;
    }
  }else if(op == GREATER_THAN_OR_EQUAL){
      if(Comparison(AMDesc, value, (void*)(data+LEAF_HEADER_SIZE+(getLength1(AMDesc) + getLength2(AMDesc)) * pos)) <= 0){
        MoveForward(scanDesc, AMDesc);
        return value2;
    }
  }

  AM_errno = AME_EOF;
  return NULL;
}


int AM_CloseIndexScan(int scanDesc) {
  if(ScanFiles[scanDesc].AMindex != -1){
    ScanFiles[scanDesc].AMindex = -1;
    ScanFiles[scanDesc].op = -1;
    ScanFiles[scanDesc].comparisonKey = NULL;
    ScanFiles[scanDesc].currentBlock = -1;
    ScanFiles[scanDesc].entryPos = -1;
  }else{
    return AM_FAILED_CLOSE_INDEX_SCAN;
  }

  return AME_OK;
}


void AM_PrintError(char *errString) {
  printf("errString: %s\n", errString);

  switch (AM_errno) {
    case AM_FULLMOP:
    printf("Array of Max Open Files is full, cannot open another file\n");

    case AM_WRONGFILE:
    printf("Attempted to open a non B+ Tree File\n");

    case AM_OPENSCANS:
    printf("Cannot close File, there are still open scans\n");

    case AM_AL_OPENED:
    printf("File is already open\n");

    case AM_ATTR_ERROR:
    printf("Incompatible attribute values\n");

    case AM_BF_ERROR:
    printf("Bf error occurred during CALL_BF\n");

    case AM_MAX_SCANS_EXCEEDED:
    printf("Can't exceed limit of 20 open scans\n");

    case AM_FAILED_INSERT:
    printf("Failed to insert entry\n");

    case AM_FAILED_CLOSE_INDEX_SCAN:
    printf("Failed to close index scan\n");

    case AM_FAILED_DESTROY_INDEX:
    printf("Failed to destroy index\n");

    case AM_FAILED_CLOSE_INDEX:
    printf("Failed to close index\n");
  }
}

void AM_Close() {
  free(OpenFiles);
  free(ScanFiles);
}
