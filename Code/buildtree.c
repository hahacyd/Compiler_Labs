#include "buildtree.h"
#include"debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
static struct TreeNode treenodepool[200];
static int nodeindex = 0;
static TreeNode* allocateTreeNode()
{
    TreeNode* b = (TreeNode*)malloc(sizeof(TreeNode));
    assert(NULL != b); // if memory allocate error,program whill assert here

    b->terminaltype = NONTERMINAL;
    b->type = EMPTY_TYPE;
    b->nodename = NULL;

    b->lineno = -1;
    b->int_num = 0;
    b->idname = NULL;
    // b->value = 0;

    b->child = NULL;
    b->sibling = NULL;

    b->isleftvalue = false;
    //lab3
    b->varible_is_arg = 0;
    return b;
}
/**
 * @brief It add a child Node
 * @param parent a Node as parent
 * @return whether "add Child" succeed
 */
TreeNode* createNode()
{
    TreeNode* t = allocateTreeNode();
    t->emptyproduce = 0;
    t->isleftvalue = 0;
    return t;
}
TreeNode* createHeadNode(char* nodename, NodeType type, int lineno)
{
    TreeNode* t = createNode();
    t->nodename = nodename;
    t->lineno = lineno;
    t->type = type;
    // TODO:这以后可以加上 列号的赋值
    return t;
}
char* allocstr(const char* str)
{
    char* t = malloc(strlen(str) + 1);
    memcpy(t, str, strlen(str));
    t[strlen(str)] = '\0';
    return t;
}
int addChild(TreeNode* parent, TreeNode* child)
{
    if (NULL == parent || NULL == child) {
        printf("\033[20;31madd Child error!parent = %p child = %p\033[0m\n", parent,
            child);
        assert(0);
        return -1;
    }
    if (NULL == parent->child) {
        parent->child = child;
    } else {
        // addSibling(parent->child, child);
        printf("\033[20;31madd Child error!parent->child = %p child = %p\033[0m\n",
            parent->child, child);
        picnic("调用此函数出错，parent不能有孩子节点");
    }
    return 1;
}
int addSibling(TreeNode* oldsibling, TreeNode* newsibling)
{
    if (oldsibling == newsibling) {
        LOG("%p ,%p\n", oldsibling, newsibling);
        return -1;
        // picnic("又是熟悉的bug odsibling 等于 newsibling");
    }
    if (oldsibling->sibling == oldsibling) {
        oldsibling->sibling = NULL;
    }
    if (NULL == newsibling->sibling && NULL != newsibling && NULL == oldsibling->sibling) {
        oldsibling->sibling = newsibling;
    }
    return 1;
}
static int times = 0;
void printTree(TreeNode* root, int layer)
{
    // usleep(100000);   //debug用
    if (NULL != root) {
        if (1 == root->emptyproduce) { //空产生式不打印
            return;
        }
        for (int i = 0; i < layer; ++i)
            printf("  "); // 按要求缩进 2 个空格
        if (NONTERMINAL == root->terminaltype) {
            printf("%s (%d)\n", root->nodename, root->lineno);
        } else {
            switch (root->type) {
            case ID:
            case TYPE:
                printf("%s: %s\n", root->nodename, root->idname);
                break;
            case INT:
                printf("%s: %d\n", root->nodename, root->int_num);
                break;
            case FLOAT:
                printf("%s: %f\n", root->nodename, root->float_num);
                break;
            default:
                printf("%s\n", root->nodename);
                break;
            }
        }

        TreeNode* temp = root->child;
        while (NULL != temp) {
            printTree(temp, layer + 1);
            temp = temp->sibling;
        }
    } else {
        picnic("never come here!");
        return;
    }
}

// lab3部分
int getIntValue(TreeNode* intnode)
{
    assert(intnode->type == INT);
    return intnode->int_num;
}
float getFloatValue(TreeNode* floatnode)
{
    assert(floatnode->type == FLOAT);
    return floatnode->float_num;
}