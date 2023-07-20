//This program supports "avl" and "scapegoat"
//Example run ./grieferList avl sample_griefers.dat < sample_input.txt
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#define LOG32(n) (log(n) / log(3.0/2.0))
 
//Tree node (both avl and scapegoat)
struct node {
    char *val;
    int height;
    int banCount, time;
    struct node *left, *right;
};

struct sg_tree {
    struct node *root;
    int total_nodes;    // Number of nodes in tree
    int q;  // Overestimate of total_nodes
};
 
//Get max between two values
int cmpMax(int a, int b) {
    int max;
    if(a > b) { max = a; }
    else { max = b; }
    return max;
}
 
//Find height of tree
int height(struct node *tree) {
    if (tree == NULL) { return 0; }
    return tree->height;
}

int sg_height(struct node* tree) {
    if(tree == NULL) { return 0; }
    return sg_height(tree->left) + sg_height(tree->right) + 1;
}

 
//Returns link of nodes in non-decreasing order
struct node *flatten(struct node *a, struct node *b) {
    if(a == NULL) { return b; }
    a->right = flatten(a->right, b);
    return flatten(a->left, a);
}

/* a = head of right child pointer list
 * with length >= n+1. Returns n+1 node "s", with 
 * left child pointing to root of newly created tree
 */ 
struct node *buildTree(struct node *a, int n) {
    if(n == 0) {
        a->left = NULL;
        return a;
    }
    double math = (n-1)/2.0;
    double mathCeil = ceil(math);
    double mathFloor = floor(math);

    struct node *r = buildTree(a, mathCeil);
    struct node *s = buildTree(r->right, mathFloor);

    r->right = s->left;
    s->left = r;
    return s;
}

// Returns the rebuilt tree rooted at temp's left child
struct node *rebuildTree(struct node *x, int n) {
    struct node *temp = (struct node*)malloc(sizeof(struct node));

    struct node *z = flatten(x, temp);
    buildTree(z, n);
    return temp->left;
}

//Create node here 
struct node *createNode(char *val, int time) {  
    //Assigning values to current node
    struct node *current = (struct node*)malloc(sizeof(struct node));

    current->val = malloc(strlen(val));
    strcpy(current->val, val);
    current->left = NULL; 
    current->right = NULL; 
    current->height = 1; //Leaf node 
    current->banCount = 1;
    current->time = time;

    return current;
}

//Rotate a subtree right
struct node *rotateRight(struct node* subRoot) {
    struct node *x = subRoot->left;
    struct node *y = x->right;

    //Rotate here
    x->right = subRoot;
    subRoot->left = y;

    //Update the height
    subRoot->height = cmpMax(height(subRoot->left), height(subRoot->right)) + 1;
    x->height = cmpMax(height(x->left), height(x->right)) + 1;

    //Root after rotating
    return x;
}

//Rotate a subtree left 
struct node *rotateLeft(struct node* subRoot) {
    struct node *x = subRoot->right;
    struct node *y = x->left;

    //Rotate here
    x->left = subRoot;
    subRoot->right = y;

    //Update the height
    subRoot->height = cmpMax(height(subRoot->left), height(subRoot->right)) + 1;
    x->height = cmpMax(height(x->left), height(x->right)) + 1;

    return x;
}

//Find balance factor of a node
int balanceFactor(struct node *node) {
    if(node == NULL) { return 0; }
    return height(node->left) - height(node->right);
}

//Insert function for avl 
struct node *avl_insert(struct node *tree, char *val, int time) {
    //avl requires a bst insert first
    if (tree == NULL) { //Spot is empty, insert here
        return createNode(val, time);
    }
    //Check if the name is already in the tree
    if(strcmp(val, tree->val) == 0) {
        (tree->banCount)++;
        // Replace time if newly inserted time is higher
        if(time > tree->time) {
            tree->time = time;
        }
        return tree;
    }

    if(strcmp(val, tree->val) < 0) { //Go left
        tree->left = avl_insert(tree->left, val, time);
    }
    else if(strcmp(val, tree->val) > 0) { //Go right
        tree->right = avl_insert(tree->right, val, time);
    }
    else { return tree; }

     //Update height and get balance factor of current node
   // tree->height = height(tree);
    tree->height = 1 + cmpMax(height(tree->left), height(tree->right));
    int BFactor = balanceFactor(tree);

    // Imbalance cases 
    //Left-left 
    if(BFactor > 1 && strcmp(val, tree->left->val) < 0) {
        return rotateRight(tree);
    }
    //Right-right 
    else if(BFactor < -1 && strcmp(val, tree->right->val) > 0) {
        return rotateLeft(tree);
    }
    //Left-right 
    else if(BFactor > 1 && strcmp(val, tree->left->val) > 0) {
        tree->left = rotateLeft(tree->left);
        return rotateRight(tree);
    }
    //Right-left
    else if(BFactor < -1 && strcmp(val, tree->right->val) < 0) {
        tree->right = rotateRight(tree->right);
        return rotateLeft(tree);
    }
    return tree;
}

//Insertion function for scapegoat
int sgInsert_helper(struct node **root, char *val, int time, int depth, double maxDepth) {
    //Scapegoat requires bst insertion first
    int n;
    if(*root == NULL) { //Spot is empty, insert here
        *root = createNode(val, time);
        if(depth > maxDepth) { return 1;}
        else { return 0; }
    }
    //Check if the name is already in the tree
    if(strcmp(val, (*root)->val) == 0) {
        ((*root)->banCount)++;
        // Replace time if newly inserted time is higher
        if(time > (*root)->time) {
            (*root)->time = time;
        }
        return 0;
    }
    if(strcmp(val, (*root)->val) < 0) { //Go left
        n = sgInsert_helper(&((*root)->left), val, time, depth+1, maxDepth);
    }
    else { //Go right
        n = sgInsert_helper(&((*root)->right), val, time, depth+1, maxDepth);   
    }

    //Tree is currently imbalanced 
    if(n > 0) {
        int parent_height;
        int sibling_height;
        int subtree_height = n;
        
        if(strcmp(val, (*root)->val) < 0) {
            sibling_height = sg_height((*root)->right);
        }
        else {
            sibling_height = sg_height((*root)->left);
        }
        
        //Height of parent
        parent_height = subtree_height + sibling_height + 1;

        //Scapegoat here (if pass)
        if(3*subtree_height > 2*parent_height) {
            *root = rebuildTree(*root, parent_height);
            return 0;
        }
        return parent_height;
    }
    return n;
}


void sg_insert(struct sg_tree *tree, char *val, int time) {
    double maxDepth = LOG32(tree->q + 1);
    int n = sgInsert_helper(&(tree->root), val, time, 0, maxDepth);

    if(n >= 0) {
        tree->total_nodes++;
        tree->q++;
    }
}

// search for a name 
int sg_search(struct sg_tree *tree, char *val) {
    struct node *node = tree->root;
    while (node) {
        //Found a match
        if (strcmp(val, node->val) == 0) {
            printf("%s was banned from %d servers. most recently on: %d\n",
                node->val, node->banCount, node->time);
            return 1;
        } //Target is < current node
        else if (strcmp(val, node->val) < 0) {
            node = node->left; //Go left
        }
        else { //Curent node is > target
            node = node->right; //Go right
        }
    }
    printf("%s is not currently banned from any servers.\n", val);
    return 0;
}

// search for a name 
int search(struct node *node, char *val) {
    while (node) {
        //Found a match
        if (strcmp(val, node->val) == 0) {
            printf("%s was banned from %d servers. most recently on: %d\n",
                node->val, node->banCount, node->time);
            return 1;
        } //Target is < current node
        else if (strcmp(val, node->val) < 0) {
            node = node->left; //Go left
        }
        else { //Curent node is > target
            node = node->right; //Go right
        }
    }
    printf("%s is not currently banned from any servers.\n", val);
    return 0;
}

//Preorder traversal of tree
void preorder(struct node *node) {
    //If tree isn't empty
    if (node != NULL) {
        printf("%s(%d) \n", node->val, node->banCount);
        preorder(node->left);
        preorder(node->right);
    }
}

int main(int argc, char const *argv[]) {
    clock_t start = clock(); //Start timer
    struct node *node = NULL; //Root starts as null

    struct sg_tree *sg_node;
    sg_node = (struct sg_tree*)malloc(32);

    //Information of banned players
    char name[16]; //Names are guarantee to be < 16 bytes
    int id;
    int time;
    FILE * fp;

    //Select griefer file (second cmd line arg)
    fp = fopen(argv[2], "r");
    if(fp == NULL) {
        printf("File doesn't exist.");
        return 0;
    }

    //Load griefer file into the tree
    while(fscanf(fp, "%s %d %d", name, &id, &time) != EOF) {
        //Avl specified 
        if(strcmp(argv[1], "avl") == 0) {
            node = avl_insert(node, name, time);
        }
        else if(strcmp(argv[1], "scapegoat") == 0){ //Otherwise scapegoat
             sg_insert(sg_node, name, time);
        } 
    }

    //Read list of names to search for
    char nameIn[256];
    while(scanf("%100s", nameIn) != EOF) {
        //Avl specified
        if(strcmp(argv[1], "avl") == 0) {
            search(node, nameIn);
        }
        else if(strcmp(argv[1], "scapegoat") == 0){ //Otherwise scapegoat
            sg_search(sg_node, nameIn);
        }
    }
    fclose(fp);

    //End timer
    double time_taken_in_seconds = (double)( clock() - start ) / CLOCKS_PER_SEC;
    double time_taken_in_microseconds = time_taken_in_seconds * 1000000.0;

    printf("total time in microseconds: %f\n", time_taken_in_microseconds);
    return 0;
}
