#ifndef __BUILDTREE_H__
#define __BUILDTREE_H__
#include "common.h"
#include "stdbool.h"
#include "syntax.tab.h"
/*void printassert(){
    printf("")
}*/

/**
 * @brief describe Node type 详细到节点的非终结符具体的类型
 */
typedef enum tagNodeType {
  /* High-level Definitions */
  Program,
  ExtDefList,
  ExtDef,
  ExtDecList,

  /* Specifiers */
  Specifier,
  StructSpecifier,
  OptTag,
  Tag,
  /* Declarators */
  VarDec,
  FunDec,
  VarList,
  ParamDec,
  /* Statements */

  CompSt,
  StmtList,
  Stmt,

  /* Local Definitions */
  DefList,
  Def,
  DecList,
  Dec,

  /* Expressions */
  Exp,
  Args,

  EMPTYPRODUCE,  //当某个非终结符产生式为空时 可能使用，
  EMPTY_TYPE,
} NodeType;
typedef enum { NONTERMINAL, TERMINAL } TerminalType;
typedef struct TreeNode {
  char *nodename;        //节点名称
  int lineno, columnno;  //行号,列号

  NodeType type;              //节点类型
  TerminalType terminaltype;  // if a Node is a terminal,then its value
                              // TERMINAL;
  char *idname;                      //仅仅在type是ID 或 type_type时有效
  struct TreeNode *child, *sibling;  //分别是指向孩子，兄弟节点的指针

  int emptyproduce;  //当此节点是非终结符且其产生式 为空时，此值为1;

  union {             //初始化为0,
    int int_num;      //当节点是整形数时使用
    float float_num;  //当节点是浮点数时使用
    VARSpecifier specifier;  //当节点是type类型时使用
    COMPARISON comparison;   //当节点是比较符时使用
    bool varible_is_arg;     //当节点是变量时使用
  };
  //用于后面语义分析，判断Exp是否是左值
  bool isleftvalue;  //初始化为 非左值，只有在 后面 的判断函数 或
                     //bison的action中可设为true;
} TreeNode;

TreeNode *createNode();  //创建节点，附带初始化，相当于构造函数；
TreeNode *createHeadNode(char *nodename, NodeType type,
                         int lineno);             //和上面的函数类似，
int addChild(TreeNode *parent, TreeNode *child);  //为parent添加child;
int addSibling(TreeNode *oldsibling,
               TreeNode *newsibling);  //为oldsibling创建新兄弟,

char *allocstr(const char *str);  //为TreeNode分配内存空间，由createNode调用,
void printTree(TreeNode *root, int layer);  //打印语法树，

//功能函数：如获取某一INT常量 语法树节点的 常量值;
int getIntValue(TreeNode *intnode);
float getFloatValue(TreeNode *floatnode);
#endif