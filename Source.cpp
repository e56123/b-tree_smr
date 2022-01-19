#include<bits/stdc++.h>
using namespace std;

#define MAX 50


#define SECTORS_PER_TRACK 1024
#define TRACKS_PER_ZONE 256
#define NUMBER_OF_ZONE 50
#define hot_zone_smr_to_ssd_threshold 64
#define ssd_to_cold_zone_smr_threshold 4096

long long* invalid_per_zone;
long long* key_count_per_zone;
//long long* lastest_zone_per_key;
//long long* lastest_pos_per_key;
//long long* lastest_crossing_zone_per_key;
//long long* lastest_crossing_pos_per_key;
//long long valid_key_in_zone[NUMBER_OF_ZONE][SECTORS_PER_TRACK * TRACKS_PER_ZONE];
long long key_in_zone[NUMBER_OF_ZONE][SECTORS_PER_TRACK * TRACKS_PER_ZONE];
long long write_pos_in_zone[NUMBER_OF_ZONE][SECTORS_PER_TRACK * TRACKS_PER_ZONE];
//int** update_key_mapping;
//int** update_key;
//number of pointers or number of child blocks [numberOfPointers = numberOfNodes + 1]
int current_write_pos = 0;
int numberOfPointers = 5;
int length_greater_than_zone = 0;
double current_time = 0;
fstream outfile;
fstream smr_outfile;
fstream ssd_outfile;

//linklist
struct Node {
    int data;
    Node* next;
};

Node* free_zone = NULL;
Node* gc_zone = NULL;
/* Function to remove the first node
   of the linked list */
Node* removeFirstNode(struct Node* head)
{
    if (head == NULL)
        return NULL;

    // Move the head pointer to the next node 
    Node* temp = head;
    head = head->next;

    delete temp;

    return head;
}

void deleteNode(Node** head_ref, int key)
{

    // Store head node
    Node* temp = *head_ref;
    Node* prev = NULL;

    // If head node itself holds
    // the key to be deleted
    if (temp != NULL && temp->data == key)
    {
        *head_ref = temp->next; // Changed head
        delete temp;            // free old head
        return;
    }

    // Else Search for the key to be deleted, 
    // keep track of the previous node as we
    // need to change 'prev->next' */
    else
    {
        while (temp != NULL && temp->data != key)
        {
            prev = temp;
            temp = temp->next;
        }

        // If key was not present in linked list
        if (temp == NULL)
            return;

        // Unlink the node from linked list
        prev->next = temp->next;

        // Free memory
        delete temp;
    }
}
int get_node(struct Node* head)
{
    if (head == NULL)
        return -1;
    else
        return head->data;
}
// Function to push node at head 
void push(struct Node** head_ref, int new_data)
{
    Node* new_node = new Node();
    new_node->data = new_data;
    new_node->next = (*head_ref);
    (*head_ref) = new_node;
}

// Function to push node at head 
void push_back(struct Node** head_ref, int new_data)
{
    Node* new_node = new Node();
    new_node->data = new_data;
    new_node->next = NULL;
    Node* current = *head_ref;
    if (*head_ref == NULL) {
        *head_ref = new_node;
        return;
    }


    while (current->next != NULL) {
        current = current->next;
    }
    current->next = new_node;
    return;
}

//zone mapping
void create_map() {
    invalid_per_zone = new long long[NUMBER_OF_ZONE];
    key_count_per_zone = new long long[NUMBER_OF_ZONE];
    /*lastest_zone_per_key = new long long[LBATOTAL + 1];
    lastest_pos_per_key = new long long[LBATOTAL + 1];
    lastest_crossing_zone_per_key = new long long[LBATOTAL + 1];
    lastest_crossing_pos_per_key = new long long[LBATOTAL + 1];*/

    for (int i = 0; i < NUMBER_OF_ZONE; i++)
        invalid_per_zone[i] = 0;

    for (int i = 0; i < NUMBER_OF_ZONE; i++)
        key_count_per_zone[i] = 0;
    for (int i = 0; i < NUMBER_OF_ZONE; i++)
        push_back(&free_zone, i);

    /*for (int i = 0; i < LBATOTAL + 1; i++)
        lastest_zone_per_key[i] = -1;
    for (int i = 0; i < LBATOTAL + 1; i++)
        lastest_pos_per_key[i] = -1;
    for (int i = 0; i < LBATOTAL + 1; i++)
        lastest_crossing_zone_per_key[i] = -1;
    for (int i = 0; i < LBATOTAL + 1; i++)
        lastest_crossing_pos_per_key[i] = -1;*/

    for (int i = 0; i < NUMBER_OF_ZONE; i++) {
        for (int j = 0; j < SECTORS_PER_TRACK * TRACKS_PER_ZONE; j++) {
            key_in_zone[i][j] = -1;
        }
    }
    for (int i = 0; i < NUMBER_OF_ZONE; i++) {
        for (int j = 0; j < SECTORS_PER_TRACK * TRACKS_PER_ZONE; j++) {
            write_pos_in_zone[i][j] = -1;
        }
    }
    /*for (int i = 0; i < NUMBER_OF_ZONE; i++) {
        for (int j = 0; j < SECTORS_PER_TRACK * TRACKS_PER_ZONE; j++) {
            valid_key_in_zone[i][j] = -1;
        }
    }*/
    /*update_key_mapping = new int*[NUMBER_OF_ZONE];
    for (int i = 0; i < NUMBER_OF_ZONE; ++i) {
        update_key_mapping[i] = new int[LBATOTAL + 1];
    }
    update_key = new int* [NUMBER_OF_ZONE];
    for (int i = 0; i < NUMBER_OF_ZONE; ++i) {
        update_key[i] = new int[LBATOTAL + 1];
    }*/
}

long long track(long long pba)
{		//Track start from 0, LBA start from 1
    if (pba == -1)
        return -1;
    long long n = (pba) / SECTORS_PER_TRACK;
    return n;
}

long long track_head(long long t)
{						// if LBA start from 1
    return t * SECTORS_PER_TRACK + 1;
}

long long zone(long long t) {
    return t / TRACKS_PER_ZONE;
}


//b+tree

struct Block {
    //number of nodes
    int tNodes;
    //zone which data cross two zones
    int crossing_zone[MAX];
    //zone which data cross two zones
    int crossing_zone_length[MAX];
    //for parent Block and index
    Block* parentBlock;
    //values
    int value[MAX];
    //length
    int data_length[MAX];
    //pba
    int write_pos[MAX];
    //previous pba
    int pre_write_pos[MAX];
    //child Blocks
    Block* childBlock[MAX];
    Block() { //constructor to initialize a block
        tNodes = 0;
        parentBlock = NULL;
        for (int i = 0; i < MAX; i++) {
            value[i] = INT_MAX;
            data_length[i] = 0;
            childBlock[i] = NULL;
            crossing_zone[i] = -1;
            crossing_zone_length[i] = 0;
            pre_write_pos[i] = -1;
        }
    }
};
//declare root Block
Block* rootBlock = new Block();


long long get_free_zone() {
    int tem = free_zone->data;
    free_zone = removeFirstNode(free_zone);
    push_back(&gc_zone, tem);
    /*if (free_zone == NULL) {
        gc_invalid_zone();
    }*/
    return free_zone->data;
}

//function to split the leaf nodes
void splitLeaf(Block* curBlock) {
    int x, i, j;

    //split the greater half to the left when numberOfPointer is odd
    //else split equal equal when numberOfPointer is even
    if (numberOfPointers % 2)
        x = (numberOfPointers + 1) / 2;
    else x = numberOfPointers / 2;

    //we don't declare another block for leftBlock, rather re-use curBlock as leftBlock and
    //take away the right half values to the rightBlock
    Block* rightBlock = new Block();

    //so leftBlock has x number of nodes
    curBlock->tNodes = x;
    //and rightBlock has numberOfPointers-x
    rightBlock->tNodes = numberOfPointers - x;
    //so both of them have their common parent [even parent may be null, so both of them will have null parent]
    rightBlock->parentBlock = curBlock->parentBlock;

    for (i = x, j = 0; i < numberOfPointers; i++, j++) {
        //take the right-half values from curBlocks and put in the rightBlock
        rightBlock->value[j] = curBlock->value[i];

        rightBlock->data_length[j] = curBlock->data_length[i];

        rightBlock->write_pos[j] = curBlock->write_pos[i];

        rightBlock->crossing_zone[j] = curBlock->crossing_zone[i];

        rightBlock->crossing_zone_length[j] = curBlock->crossing_zone_length[i];

        rightBlock->pre_write_pos[j] = curBlock->pre_write_pos[i];
        //and erase right-half values from curBlock to make it real leftBlock
        //so that it does not contain all values only contains left-half values
        curBlock->value[i] = INT_MAX;

        //curBlock->data_length[i] = INT_MAX;

       // curBlock->write_pos[i] = INT_MAX;

        //curBlock->crossing_zone[i] = INT_MAX;

        //curBlock->crossing_zone_length[i] = INT_MAX;
    }
    //for splitting the leaf blocks we copy the first item from the rightBlock to their parentBlock
    //and val contains that value
    int val = rightBlock->value[0];

    int length = rightBlock->data_length[0];

    int temp_write_pos = rightBlock->write_pos[0];

    int temp_crossing_zone = rightBlock->crossing_zone[0];

    int temp_crossing_zone_length = rightBlock->crossing_zone_length[0];

    int temp_pre_write_pos = rightBlock->pre_write_pos[0];
    //if the leaf itself is a parent then
    if (curBlock->parentBlock == NULL) {
        //it has null parent, so create a new parent
        Block* parentBlock = new Block();
        //and new parent should have a null parent
        parentBlock->parentBlock = NULL;
        //new parent will have only one member
        parentBlock->tNodes = 1;
        //and that member is val
        parentBlock->value[0] = val;

        parentBlock->data_length[0] = length;

        parentBlock->write_pos[0] = temp_write_pos;

        parentBlock->crossing_zone[0] = temp_crossing_zone;

        parentBlock->crossing_zone_length[0] = temp_crossing_zone_length;

        parentBlock->pre_write_pos[0] = temp_pre_write_pos;
        //
        //so the parent has two child, so assign them (don't forget curBlock is actually the leftBlock)
        parentBlock->childBlock[0] = curBlock;
        parentBlock->childBlock[1] = rightBlock;
        //their parent of the left and right blocks is no longer null, so assign their parent
        curBlock->parentBlock = rightBlock->parentBlock = parentBlock;
        //from now on this parentBlock is the rootBlock
        rootBlock = parentBlock;
        return;
    }
    else {   //if the splitted leaf block is not rootBlock then

        // we have to put the val and assign the rightBlock to the right place in the parentBlock
        // so we go to the parentBlock and from now we consider the curBlock as the parentBlock of the splitted Block

        curBlock = curBlock->parentBlock;

        //for the sake of insertNodeion sort to put the rightBlock and val in the exact position
        //of th parentBlock [here curBlock] take a new child block and assign rightBlock to it
        Block* newChildBlock = new Block();
        newChildBlock = rightBlock;

        //simple insertion sort to put val at the exact position of values[] in the parentBlock [here curBlock]

        for (i = 0; i <= curBlock->tNodes; i++) {
            if (val < curBlock->value[i]) {
                swap(curBlock->value[i], val);

                swap(curBlock->data_length[i], length);

                swap(curBlock->write_pos[i], temp_write_pos);

                swap(curBlock->crossing_zone[i], temp_crossing_zone);

                swap(curBlock->crossing_zone_length[i], temp_crossing_zone_length);

                swap(curBlock->pre_write_pos[i], temp_pre_write_pos);
            }
        }

        //after putting val number of nodes gets increase by one
        curBlock->tNodes++;

        //simple insertNodeion sort to put rightBlock at the exact position
        //of childBlock[] in the parentBlock [here curBlock]

        for (i = 0; i < curBlock->tNodes; i++) {
            if (newChildBlock->value[0] < curBlock->childBlock[i]->value[0]) {
                swap(curBlock->childBlock[i], newChildBlock);
            }
        }
        curBlock->childBlock[i] = newChildBlock;

        //we reordered some blocks and pointers, so for the sake of safety
        //all childBlocks' should have their parent updated
        for (i = 0; curBlock->childBlock[i] != NULL; i++) {
            curBlock->childBlock[i]->parentBlock = curBlock;
        }
    }

}

//function to split the non leaf nodes
void splitNonLeaf(Block* curBlock) {
    int x, i, j;

    //split the less half to the left when numberOfPointer is odd
    //else split equal equal when numberOfPointer is even.  n/2 does it nicely for us

    x = numberOfPointers / 2;

    //declare rightBlock and we will use curBlock as the leftBlock
    Block* rightBlock = new Block();

    //so leftBlock has x number of nodes
    curBlock->tNodes = x;
    //rightBlock has numberOfPointers-x-1 children, because we won't copy and paste
    //rather delete and paste the first item of the rightBlock
    rightBlock->tNodes = numberOfPointers - x - 1;
    //both children have their common parent
    rightBlock->parentBlock = curBlock->parentBlock;


    for (i = x, j = 0; i <= numberOfPointers; i++, j++) {
        //copy the right-half members to the rightBlock
        rightBlock->value[j] = curBlock->value[i];

        rightBlock->data_length[j] = curBlock->data_length[i];

        rightBlock->write_pos[j] = curBlock->write_pos[i];

        rightBlock->crossing_zone[j] = curBlock->crossing_zone[i];

        rightBlock->crossing_zone_length[j] = curBlock->crossing_zone_length[i];

        rightBlock->pre_write_pos[j] = curBlock->pre_write_pos[i];
        //and also copy their children
        rightBlock->childBlock[j] = curBlock->childBlock[i];

        //
        //erase the right-half values from curBlock to make it perfect leftBlock
        //which won't contain only left-half values and their children
        curBlock->value[i] = INT_MAX;

        // curBlock->data_length[i] = INT_MAX;

        // curBlock->write_pos[i] = INT_MAX;

        // curBlock->crossing_zone[i] = INT_MAX;

        // curBlock->crossing_zone_length[i] = INT_MAX;
         //erase all the right-half childBlocks from curBlock except the x one
         //because if left child has 3 nodes then it should have 4 childBlocks, so don't delete that child
        if (i != x)curBlock->childBlock[i] = NULL;
    }

    //we will take a copy of the first item of the rightBlock
    //as we will delete that item later from the list
    int val = rightBlock->value[0];

    int length = rightBlock->data_length[0];

    int tem_write_pos = rightBlock->write_pos[0];

    int temp_crossing_zone = rightBlock->crossing_zone[0];

    int temp_crossing_zone_length = rightBlock->crossing_zone_length[0];

    int temp_pre_write_pos = rightBlock->pre_write_pos[0];
    //just right-shift value[] and childBlock[] by one from rightBlock
    //to have no repeat of the first item for non-leaf Block


    memcpy(&rightBlock->value, &rightBlock->value[1], sizeof(int) * (rightBlock->tNodes + 1));

    memcpy(&rightBlock->data_length, &rightBlock->data_length[1], sizeof(int) * (rightBlock->tNodes + 1));

    memcpy(&rightBlock->write_pos, &rightBlock->write_pos[1], sizeof(int) * (rightBlock->tNodes + 1));

    memcpy(&rightBlock->crossing_zone, &rightBlock->crossing_zone[1], sizeof(int) * (rightBlock->tNodes + 1));

    memcpy(&rightBlock->crossing_zone_length, &rightBlock->crossing_zone_length[1], sizeof(int) * (rightBlock->tNodes + 1));

    memcpy(&rightBlock->pre_write_pos, &rightBlock->pre_write_pos[1], sizeof(int) * (rightBlock->tNodes + 1));

    memcpy(&rightBlock->childBlock, &rightBlock->childBlock[1], sizeof(rootBlock) * (rightBlock->tNodes + 1));
    //length


    //we reordered some values and positions so don't forget
    //to assign the children's exact parent

    for (i = 0; curBlock->childBlock[i] != NULL; i++) {
        curBlock->childBlock[i]->parentBlock = curBlock;
    }
    for (i = 0; rightBlock->childBlock[i] != NULL; i++) {
        rightBlock->childBlock[i]->parentBlock = rightBlock;
    }

    //if the splitted block itself a parent
    if (curBlock->parentBlock == NULL) {
        //create a new parent
        Block* parentBlock = new Block();
        //parent should have a null parent
        parentBlock->parentBlock = NULL;
        //parent will have only one node
        parentBlock->tNodes = 1;
        //the only value is the val
        parentBlock->value[0] = val;

        parentBlock->data_length[0] = length;

        parentBlock->write_pos[0] = tem_write_pos;

        parentBlock->crossing_zone[0] = temp_crossing_zone;

        parentBlock->crossing_zone_length[0] = temp_crossing_zone_length;

        parentBlock->pre_write_pos[0] = temp_pre_write_pos;

        //
        //it has two children, leftBlock and rightBlock
        parentBlock->childBlock[0] = curBlock;
        parentBlock->childBlock[1] = rightBlock;

        //and both rightBlock and leftBlock has no longer null parent, they have their new parent
        curBlock->parentBlock = rightBlock->parentBlock = parentBlock;

        //from now on this new parent is the root parent
        rootBlock = parentBlock;
        return;
    }
    else {   //if the splitted leaf block is not rootBlock then

        // we have to put the val and assign the rightBlock to the right place in the parentBlock
        // so we go to the parentBlock and from now we consider the curBlock as the parentBlock of the splitted Block
        curBlock = curBlock->parentBlock;

        //for the sake of insertNodeion sort to put the rightBlock and val in the exact position
        //of th parentBlock [here curBlock] take a new child block and assign rightBlock to it

        Block* newChildBlock = new Block();
        newChildBlock = rightBlock;
        tem_write_pos = current_write_pos;

        //simple insertion sort to put val at the exact position of values[] in the parentBlock [here curBlock]


        for (i = 0; i <= curBlock->tNodes; i++) {
            if (val < curBlock->value[i]) {
                swap(curBlock->value[i], val);

                swap(curBlock->data_length[i], length);

                swap(curBlock->write_pos[i], tem_write_pos);

                swap(curBlock->crossing_zone[i], temp_crossing_zone);

                swap(curBlock->crossing_zone_length[i], temp_crossing_zone_length);

                swap(curBlock->pre_write_pos[i], temp_pre_write_pos);
            }
        }

        //after putting val number of nodes gets increase by one
        curBlock->tNodes++;

        //simple insertNodeion sort to put rightBlock at the exact position
         //of childBlock[] in the parentBlock [here curBlock]

        for (i = 0; i < curBlock->tNodes; i++) {
            if (newChildBlock->value[0] < curBlock->childBlock[i]->value[0]) {
                swap(curBlock->childBlock[i], newChildBlock);
            }
        }
        curBlock->childBlock[i] = newChildBlock;

        //we reordered some blocks and pointers, so for the sake of safety
        //all childBlocks' should have their parent updated
        for (i = 0; curBlock->childBlock[i] != NULL; i++) {
            curBlock->childBlock[i]->parentBlock = curBlock;
        }
    }

}

//
int return_node = 0;
Block* searchNode(Block* curBlock, int val) {
    for (int i = 0; i <= curBlock->tNodes; i++) {
        if (val < curBlock->value[i] && curBlock->childBlock[i] != NULL) {
            return searchNode(curBlock->childBlock[i], val);
        }
        else if (val == curBlock->value[i] && curBlock->childBlock[i] == NULL) {
            return_node = i;
            return curBlock;
        }
    }
    return NULL;

}
struct access {
    double time;
    char iotype;
    long long address;  //lba, may be pba when write file
//	long long pba = 0;
    int size;
    int device = 0;

};

void gc_invalid_zone() {
    int most_invalid = -1;//invalid number
    int zone_of_most_invalid = 0;//invalid zone
    for (Node* temp = gc_zone; temp != NULL; temp = temp->next) {
        if (most_invalid < invalid_per_zone[temp->data]) {
            most_invalid = invalid_per_zone[temp->data];
            zone_of_most_invalid = temp->data;
        }
    }
   // cout << "OK" <<endl;
    invalid_per_zone[zone_of_most_invalid] = 0;
    push_back(&free_zone, zone_of_most_invalid);
    deleteNode(&gc_zone, zone_of_most_invalid);
    int current_zone = zone(track(current_write_pos));
    access temp_reading;
    access temp_writing;
    vector <access> reading;
    vector <access> writing;
    /*cout << "CURRENT_WRITE_POS_IN_GC: "<< current_write_pos << endl;
    cout << zone_of_most_invalid<<" "<<key_count_per_zone[zone_of_most_invalid] << endl;
    cout << "free_zone: ";
    for (Node* temp = free_zone; temp != NULL; temp = temp->next)
        cout << temp->data << " ";
    cout << endl;
    cout << "gc_zone: ";
    for (Node* temp = gc_zone; temp != NULL; temp = temp->next)
        cout << temp->data << " ";
    cout << endl;
    cout << "invalid_length_per_zone: ";
    for (int i = 0; i < NUMBER_OF_ZONE; i++) {
        cout << invalid_per_zone[i] << ' ';
    }
    cout << endl;*/
    for (int i = 0; i < key_count_per_zone[zone_of_most_invalid]-1; i++) {
        //cout << "gc_operation" << endl;
        Block* tem = searchNode(rootBlock, key_in_zone[zone_of_most_invalid][i]);
       // cout << key_in_zone[zone_of_most_invalid][i] <<" " <<tem->value[return_node]<< endl;
        //cout << write_pos_in_zone[zone_of_most_invalid][i]<<" "<<tem->write_pos[return_node] << ' ' << tem->data_length[return_node] << endl;
        //cout << "OK" << endl;
        if (tem->write_pos[return_node] == write_pos_in_zone[zone_of_most_invalid][i]) {//valid data
            if (tem->crossing_zone[return_node] != -1) {

                tem->crossing_zone[return_node] = current_zone;
                current_write_pos += tem->crossing_zone_length[return_node];

                //manage value position in zone freezone在哪裡處理 i 修成vilid數量
                key_in_zone[current_zone][i] = tem->value[return_node];
                write_pos_in_zone[current_zone][i] = tem->write_pos[return_node];
                key_count_per_zone[current_zone]++;
            }
            else {
                //cout << "OK" << endl;
                tem->write_pos[return_node] = current_write_pos;
                current_write_pos += (tem->data_length[return_node]-tem->crossing_zone_length[return_node]);

                //manage value position in zone
                key_in_zone[current_zone][i] = tem->value[return_node];
                write_pos_in_zone[current_zone][i] = tem->write_pos[return_node];
                key_count_per_zone[current_zone]++;

            }
            temp_reading.iotype = 'R';
            temp_reading.address = tem->value[return_node];
            temp_reading.time = current_time;
            temp_reading.size = tem->data_length[return_node];
            reading.push_back(temp_reading);

            temp_writing.iotype = 'W';
            temp_writing.address = tem->value[return_node];
            temp_writing.time = current_time;
            temp_writing.size = tem->data_length[return_node];
            writing.push_back(temp_writing);
        }
       
    }
  
    for (int i = 0; i < reading.size(); i++) {
        if (reading[i].size > hot_zone_smr_to_ssd_threshold && reading[i].size < ssd_to_cold_zone_smr_threshold) {
            ssd_outfile << reading[i].time << "\t" << "0" << "\t" <<
                reading[i].address << '\t' << reading[i].size << "\t" << "1" << endl;
            outfile << reading[i].time << "\t" << "0" << "\t" <<
                reading[i].address << '\t' << reading[i].size << "\t" << "1" << endl;
        }
        else {
            smr_outfile << reading[i].time << "\t" << "0" << "\t" <<
                reading[i].address << '\t' << reading[i].size << "\t" << "0" << endl;
            outfile << reading[i].time << "\t" << "0" << "\t" <<
                reading[i].address << '\t' << reading[i].size << "\t" << "0" << endl;
        }
        
    }
    for (int i = 0; i < writing.size(); i++) {
        if (writing[i].size > hot_zone_smr_to_ssd_threshold && writing[i].size < ssd_to_cold_zone_smr_threshold) {
            ssd_outfile << writing[i].time << "\t" << "1" << "\t" <<
                writing[i].address << '\t' << writing[i].size << "\t" << "1" << endl;
            outfile << writing[i].time << "\t" << "1" << "\t" <<
                writing[i].address << '\t' << writing[i].size << "\t" << "1" << endl;
        }
        else {
            smr_outfile << writing[i].time << "\t" << "1" << "\t" <<
                writing[i].address << '\t' << writing[i].size << "\t" << "0" << endl;
            outfile << writing[i].time << "\t" << "1" << "\t" <<
                writing[i].address << '\t' << writing[i].size << "\t" << "0" << endl;
        }
        
    }
    /*for (int i = 0; i < reading.size(); i++) {
        cout << reading[i].iotype << ' ' << reading[i].address << ' ' << reading[i].size << endl;
    }
    for (int i = 0; i < writing.size(); i++) {
        cout << writing[i].iotype << ' ' << writing[i].address << ' ' << writing[i].size << endl;
    }*/
    for (int i = 0; i < key_count_per_zone[zone_of_most_invalid]; i++) {
        key_in_zone[zone_of_most_invalid][i] = -1;
        write_pos_in_zone[zone_of_most_invalid][i] = -1;
       // valid_key_in_zone[zone_of_most_invalid][i] = -1;
    }
    key_count_per_zone[zone_of_most_invalid] = 0;
    /*search_same_zone_key(rootBlock, zone_of_most_invalid);
    cout << "gc_key: ";
    for (int i = 0; i < gc_key.size(); i++) {
        cout << *gc_key[i] << ' ';
    }

    cout << endl;*/
}
void updateNode(Block* curBlock, int val, int length) {

   /*f (lastest_zone_per_key[val] != -1 && lastest_pos_per_key[val] != -1)
        valid_key_in_zone[lastest_zone_per_key[val]][lastest_pos_per_key[val]] = -1;
    if (lastest_crossing_zone_per_key[val] != -1 && lastest_crossing_pos_per_key[val] != -1)
        valid_key_in_zone[lastest_crossing_zone_per_key[val]][lastest_crossing_pos_per_key[val]] = -1;*/

    int current_zone = zone(track(current_write_pos));

    //valid_key_in_zone[current_zone][key_count_per_zone[current_zone]] = 1;
    key_in_zone[current_zone][key_count_per_zone[current_zone]] = val;
    write_pos_in_zone[current_zone][key_count_per_zone[current_zone]] = current_write_pos;
    /*lastest_zone_per_key[val] = current_zone;
    lastest_pos_per_key[val] = key_count_per_zone[current_zone];*/

    key_count_per_zone[current_zone]++;


    //curblock is searched block
    int tem_write_pos = current_write_pos;
    //length over a zone
    int pre_length_greater_than_zone =
        curBlock->crossing_zone_length[return_node];

    //if updating node is between two zones
    if (curBlock->crossing_zone[return_node] != -1) {
        invalid_per_zone[curBlock->crossing_zone[return_node]] += pre_length_greater_than_zone;
        invalid_per_zone[zone(track(curBlock->write_pos[return_node]))]
            += (curBlock->data_length[return_node] - pre_length_greater_than_zone);
    }
    //if updating node is in one zone
    else
        invalid_per_zone[zone(track(curBlock->write_pos[return_node]))] += curBlock->data_length[return_node];

    //block data update
    int tem_index = return_node;
    if (zone(track(current_write_pos)) != zone(track(current_write_pos + length))) {
        int length_greater_than_zone =
            length - (SECTORS_PER_TRACK * TRACKS_PER_ZONE - (current_write_pos % (SECTORS_PER_TRACK * TRACKS_PER_ZONE)));
        int FREE_ZONE = get_free_zone();
        current_write_pos = track_head(FREE_ZONE * TRACKS_PER_ZONE) - 1;
        if (free_zone->next == NULL) {
            gc_invalid_zone();
            //cout << FREE_ZONE << endl;
            //cout << length_greater_than_zone << endl;
            //cout << current_write_pos << endl;
            if (FREE_ZONE != zone(track(current_write_pos + length_greater_than_zone))) {
                //cout << "disk is full" << endl;
                exit(1);
            }
        }
        //valid_key_in_zone[FREE_ZONE][key_count_per_zone[FREE_ZONE]] = 1;
        key_in_zone[FREE_ZONE][key_count_per_zone[FREE_ZONE]] = val;
        write_pos_in_zone[FREE_ZONE][key_count_per_zone[FREE_ZONE]] = current_write_pos;
        /*lastest_crossing_zone_per_key[val] = FREE_ZONE;
        lastest_crossing_pos_per_key[val] = key_count_per_zone[FREE_ZONE];*/
        key_count_per_zone[FREE_ZONE]++;


        current_write_pos = current_write_pos + length_greater_than_zone;
        swap(curBlock->crossing_zone[tem_index], FREE_ZONE);
        swap(curBlock->crossing_zone_length[tem_index], length_greater_than_zone);

    }
    else {
        current_write_pos = current_write_pos + length;
        curBlock->crossing_zone[tem_index] = -1;
        curBlock->crossing_zone_length[tem_index] = 0;
    }
    //
    swap(curBlock->data_length[tem_index], length);
    swap(curBlock->write_pos[tem_index], tem_write_pos);

}
void insertNode(Block* curBlock, int val, int length) {
    int tem_write_pos = current_write_pos;
    int tem_length = length;
    int tem_length_greater_than_zone = length_greater_than_zone;
    length_greater_than_zone = 0;
    for (int i = 0; i <= curBlock->tNodes; i++) {
        if (val < curBlock->value[i] && curBlock->childBlock[i] != NULL) {
            insertNode(curBlock->childBlock[i], val, length);
            if (curBlock->tNodes == numberOfPointers)
                splitNonLeaf(curBlock);
            return;
        }
        else if (val < curBlock->value[i] && curBlock->childBlock[i] == NULL) {
            //record every key in zone
            
            /*if (lastest_zone_per_key[val] != -1 && lastest_pos_per_key[val] != -1)
                valid_key_in_zone[lastest_zone_per_key[val]][lastest_pos_per_key[val]] = -1;
            if (lastest_crossing_zone_per_key[val] != -1 && lastest_crossing_pos_per_key[val] != -1)
                valid_key_in_zone[lastest_crossing_zone_per_key[val]][lastest_crossing_pos_per_key[val]] = -1;*/

            int current_zone = zone(track(current_write_pos));
            //valid_key_in_zone[current_zone][key_count_per_zone[current_zone]] = 1;
            key_in_zone[current_zone][key_count_per_zone[current_zone]] = val;
            write_pos_in_zone[current_zone][key_count_per_zone[current_zone]] = current_write_pos;
            /*lastest_zone_per_key[val] = current_zone;
            lastest_pos_per_key[val] = key_count_per_zone[current_zone];*/
            key_count_per_zone[current_zone]++;

            //block data update
            if (zone(track(current_write_pos)) != zone(track(current_write_pos + length))) {
                length_greater_than_zone =
                    length - (SECTORS_PER_TRACK * TRACKS_PER_ZONE - (current_write_pos % (SECTORS_PER_TRACK * TRACKS_PER_ZONE)));
                tem_length_greater_than_zone = length_greater_than_zone;
                //cout << "CURRENT_ZONE: " << current_zone << endl;
                int FREE_ZONE = get_free_zone();
                current_write_pos = track_head(FREE_ZONE * TRACKS_PER_ZONE) - 1;
                if (free_zone->next == NULL) {
                    gc_invalid_zone();                 
                    if (FREE_ZONE != zone(track(current_write_pos + length_greater_than_zone))) {
                        //cout << "disk is full" << endl;
                        exit(1);
                    }
                }
                key_in_zone[FREE_ZONE][key_count_per_zone[FREE_ZONE]] = val;
                write_pos_in_zone[FREE_ZONE][key_count_per_zone[FREE_ZONE]] = current_write_pos;    
                key_count_per_zone[FREE_ZONE]++;
                //current_write_pos = current_write_pos + length_greater_than_zone;
                swap(curBlock->crossing_zone[i], FREE_ZONE);
                swap(curBlock->crossing_zone_length[i], length_greater_than_zone);

            }
           /* else {
                current_write_pos = current_write_pos + length;
                cout << "length: "<< length << endl;
            }*/
            swap(curBlock->value[i], val);
            swap(curBlock->data_length[i], length);
            swap(curBlock->write_pos[i], tem_write_pos);
            //swap(curBlock->childBlock[i], newChildBlock);       
            if (i == curBlock->tNodes) {
                curBlock->tNodes++;
                break;
            }
        }
    }

    current_write_pos = current_write_pos + tem_length;
    //cout << "current_pos: " << current_write_pos << endl;
    //cout << "length: " << tem_length << endl;
    if (length_greater_than_zone != 0) {
        current_write_pos = current_write_pos + tem_length_greater_than_zone;
    }

    if (curBlock->tNodes == numberOfPointers) {

        splitLeaf(curBlock);
    }

}



void mergeBlock(Block* leftBlock, Block* rightBlock, bool isLeaf, int posOfRightBlock) {

    //cout << "leftBlock " << leftBlock->value[0] << " rightBlock " << rightBlock->value[0] << endl;
    //cout << "size " << leftBlock->tNodes << " size " << rightBlock->tNodes << endl;
    if (!isLeaf) {

        leftBlock->value[leftBlock->tNodes] = leftBlock->parentBlock->value[posOfRightBlock - 1];

        leftBlock->data_length[leftBlock->tNodes] = leftBlock->parentBlock->data_length[posOfRightBlock - 1];

        leftBlock->write_pos[leftBlock->tNodes] = leftBlock->parentBlock->write_pos[posOfRightBlock - 1];

        leftBlock->tNodes++;
    }

    memcpy(&leftBlock->value[leftBlock->tNodes], &rightBlock->value[0], sizeof(int) * (rightBlock->tNodes + 1));

    memcpy(&leftBlock->data_length[leftBlock->tNodes], &rightBlock->data_length[0], sizeof(int) * (rightBlock->tNodes + 1));

    memcpy(&leftBlock->write_pos[leftBlock->tNodes], &rightBlock->write_pos[0], sizeof(int) * (rightBlock->tNodes + 1));

    memcpy(&leftBlock->childBlock[leftBlock->tNodes], &rightBlock->childBlock[0], sizeof(rootBlock) * (rightBlock->tNodes + 1));




    leftBlock->tNodes += rightBlock->tNodes;


    //cout << "before: " << leftBlock->parentBlock->value[1] << endl;
    memcpy(&leftBlock->parentBlock->value[posOfRightBlock - 1], &leftBlock->parentBlock->value[posOfRightBlock], sizeof(int) * (leftBlock->parentBlock->tNodes + 1));

    memcpy(&leftBlock->parentBlock->data_length[posOfRightBlock - 1], &leftBlock->parentBlock->data_length[posOfRightBlock], sizeof(int) * (leftBlock->parentBlock->tNodes + 1));

    memcpy(&leftBlock->parentBlock->write_pos[posOfRightBlock - 1], &leftBlock->parentBlock->write_pos[posOfRightBlock], sizeof(int) * (leftBlock->parentBlock->tNodes + 1));

    memcpy(&leftBlock->parentBlock->childBlock[posOfRightBlock], &leftBlock->parentBlock->childBlock[posOfRightBlock + 1], sizeof(rootBlock) * (leftBlock->parentBlock->tNodes + 1));


    //cout << "after merging " << leftBlock->parentBlock->childBlock[posOfRightBlock-2]->value[0] << " and ";// << leftBlock->parentBlock->childBlock[posOfRightBlock]->value[0] << endl;
    leftBlock->parentBlock->tNodes--;

    //we reordered some blocks and pointers, so for the sake of safety
    //all childBlocks' should have their parent updated
    for (int i = 0; leftBlock->childBlock[i] != NULL; i++) {
        leftBlock->childBlock[i]->parentBlock = leftBlock;
    }



}


void print(vector < Block* > Blocks) {
    vector < Block* > newBlocks;
    for (int i = 0; i < Blocks.size(); i++) { //for every block
        Block* curBlock = Blocks[i];

        cout << "[|";
        int j;
        for (j = 0; j < curBlock->tNodes; j++) {  //traverse the childBlocks, print values and save all the childBlocks
            cout << curBlock->value[j] << ' ' << curBlock->data_length[j] << ' ' << curBlock->write_pos[j] << " " << curBlock->crossing_zone[j] << " " << curBlock->crossing_zone_length[j] << "|";
            if (curBlock->childBlock[j] != NULL)
                newBlocks.push_back(curBlock->childBlock[j]);
        }
        if (curBlock->value[j] == INT_MAX && curBlock->childBlock[j] != NULL)
            newBlocks.push_back(curBlock->childBlock[j]);

        cout << "]  ";
    }

    if (newBlocks.size() == 0) { //if there is no childBlock block left to send out then just the end of the recursion

        puts("");
        puts("");
        Blocks.clear();
        //exit(0);
    }
    else {                    //else send the childBlocks to the recursion to continue to the more depth
        puts("");
        puts("");
        Blocks.clear();
        print(newBlocks);
    }
}

int main() {
    int num;// = {1,4,7,10,17,21,31,25,19,20,28,42};
    int length;
    // int num[] = {5,10,15,20,25,28,30,50,55,60,65,70,75,80,85,90,95};


    //printf("Pointers: ");
    //scanf_s("%d", &numberOfPointers);
    create_map();
    FILE* p;
    // p = fopen_s("input.txt", "r");
     //freopen_s("output.txt", "w", stdout);


    /*vector < Block* > Blocks;


    char ch;
    int i = 0;
    int totalValues = 0;*/

    fstream infile;

    infile.open("in.txt", ios::in);	//input file
    if (!infile) {
        cout << "cannot find file!";
        exit(1);
    }
    else
        cout << "file successfully open" << endl;
    outfile.open("out.txt", ios::out);	//output file
    smr_outfile.open("smr_out.txt", ios::out);	//output file
    ssd_outfile.open("ssd_out.txt", ios::out);	//output file

    vector < Block* > Blocks;

  
    char iotype = 0;
    long long address = 0;
    int size = 0;
    int device = 0;
    int ssd_device = 1;
    while (infile >> iotype >> address >> size) {
        if (iotype == 'R') {
            Block* temp_block = searchNode(rootBlock, address);
            //cout << "READ" << endl;
            if (temp_block != NULL) {
                if (temp_block->data_length[return_node] < size) {
                    if (size > hot_zone_smr_to_ssd_threshold && size < ssd_to_cold_zone_smr_threshold)
                        if (size % 8 != 0)
                            size = size + (8 - size % 8);
                    updateNode(temp_block, address, size);
                    //cout << "NULL" << endl;
                    current_time += 0.1;
                    if (size > hot_zone_smr_to_ssd_threshold && size < ssd_to_cold_zone_smr_threshold) {
                        //cout << size << endl;
                        ssd_outfile << current_time << "\t" << "0" << "\t" << address << '\t' << size << "\t" << ssd_device << endl;//r
                        outfile << current_time << "\t" << "0" << "\t" << address << '\t' << size << "\t" << ssd_device << endl;//r
                    }
                    else {
                        smr_outfile << current_time << "\t" << "0" << "\t" << address << '\t' << size << "\t" << device << endl;//r
                        outfile << current_time << "\t" << "0" << "\t" << address << '\t' << size << "\t" << device << endl;//r
                    }
                    //cout << current_write_pos << endl;
                }
                /*else if (temp_block->data_length[return_node] > size) {
                    cout << "R" << endl;//do nothing
                }*/
            }
            else if (temp_block == NULL) {
                if (size > hot_zone_smr_to_ssd_threshold && size < ssd_to_cold_zone_smr_threshold)
                    if (size % 8 != 0)
                        size = size + (8 - size % 8);
                insertNode(rootBlock, address, size);
                //cout << "W" << endl;
                current_time += 0.1;
                if (size > hot_zone_smr_to_ssd_threshold && size < ssd_to_cold_zone_smr_threshold) {
                    ssd_outfile << current_time << "\t" << "1" << "\t" << address << '\t' << size << "\t" << ssd_device << endl;//w
                    outfile << current_time << "\t" << "1" << "\t" << address << '\t' << size << "\t" << ssd_device << endl;//w
                }
                else {
                    smr_outfile << current_time << "\t" << "1" << "\t" << address << '\t' << size << "\t" << device << endl;//w
                    outfile << current_time << "\t" << "1" << "\t" << address << '\t' << size << "\t" << device << endl;//w
                }
                //cout << current_write_pos << endl;

                //cout << "R" << endl;
                current_time += 0.1;
                if (size > hot_zone_smr_to_ssd_threshold && size < ssd_to_cold_zone_smr_threshold) {
                    ssd_outfile << current_time << "\t" << "0" << "\t" << address << '\t' << size << "\t" << ssd_device << endl;//r
                    outfile << current_time << "\t" << "0" << "\t" << address << '\t' << size << "\t" << ssd_device << endl;//r
                }
                else {
                    smr_outfile << current_time << "\t" << "0" << "\t" << address << '\t' << size << "\t" << device << endl;//r
                    outfile << current_time << "\t" << "0" << "\t" << address << '\t' << size << "\t" << device << endl;//r
                }
                //cout << current_write_pos << endl;
            }
        }
        else if (iotype == 'W') {
            if (size > hot_zone_smr_to_ssd_threshold && size < ssd_to_cold_zone_smr_threshold)
                if(size % 8 != 0)
                    size = size + (8 - size % 8);
            Block* temp_block = searchNode(rootBlock, address);
            //cout << "WRITE" << endl;
            if (temp_block != NULL) {
                updateNode(temp_block, address, size);
            }
            else if (temp_block == NULL) {
                insertNode(rootBlock, address, size);
            }
            current_time += 0.1;
            if (size > hot_zone_smr_to_ssd_threshold && size < ssd_to_cold_zone_smr_threshold) {
                size = size + size % 8;
                ssd_outfile << current_time << "\t" << "1" << "\t" << address << '\t' << size << "\t" << ssd_device << endl;//w
                outfile << current_time << "\t" << "1" << "\t" << address << '\t' << size << "\t" << ssd_device << endl;//w
            }
            else {
                smr_outfile << current_time << "\t" << "1" << "\t" << address << '\t' << size << "\t" << device << endl;//w
                outfile << current_time << "\t" << "1" << "\t" << address << '\t' << size << "\t" << device << endl;//w
            }
            //cout << "W" << endl;
            //cout << current_write_pos << endl;
        }
    }
    infile.close();
    outfile.close();
    for (int i = 0; i < NUMBER_OF_ZONE; i++) {
        cout << invalid_per_zone[i] << ' ';
    }
    return 0;
}