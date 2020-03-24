#include "generateIR.h"
#include "debug.h"
#include "include.h"
#include <stdio.h>
#include <string.h>
/**
 * 中间的形参 operator 是为了优化 例如 int i = 1 + 3;这样的式子的。
 * 在末尾直接翻译为 i := #1 + #3, 而不是翻译为:
 * t0 := #1 + #3
 * i := t0
 */
InterCodes* translate_Exp(TreeNode* exp, OperandOperator operator, Operand* op)
{
    assert(op != NULL);

    int intvalue = 0;
    float floatvalue = 0.0;
    TreeNode* re = NULL;
    TreeNode *exp1 = NULL, *exp2 = NULL;
    TreeNode* args = NULL;
    TreeNode* struct_id = NULL;
    Operand op1, op2, result;
    TreeNode* node = exp->child;
    enum { exp_number,
        exp_cond //如果类型是 exp_cond，说明这个exp只能是 0 或 1,是一个状态量，用translate_Cond来解析;
    } exp_type;
    exp_type = exp_number;
    Operand *array_address = NULL, *arrayoffset = NULL;
    char arraytemp[32];
    switch (exp->child->type) {
    case INT:
        intvalue = getIntValue(exp->child);
        if (operator== ASSIGNOP) {
            op1.kind = OCONST;
            op1.u.value = intvalue;

            addTwoInter(ASSIGNOP, op, &op1);
        } else {
            op->kind = OCONST;
            op->u.value = intvalue;
        }
        break;
    case FLOAT:
        floatvalue = getFloatValue(exp->child);
        break;
    case MINUS:
        translate_Exp(node->sibling, NONE, &op2);
        op1.kind = OCONST;
        op1.u.value = 0;
        if (operator== ASSIGNOP) {
        } else {
            *op = *newTemp();
        }
        addThreeInter(MINUS, op, &op1, &op2);
        break;
    case ID:
        if (node->sibling == NULL) {
            //Exp 是一个变量
            if (operator== ASSIGNOP) {
                op1.kind = OVAR;
                strcpy(op1.u.varname, node->idname);
                if (!isOperandEqual(op, &op1)) {
                    addTwoInter(ASSIGNOP, op, &op1);
                }
            } else {
                op->kind = OVAR;
                strcpy(op->u.varname, node->idname);
            }
        } else {
            //Exp 是一个函数调用
            if (strcmp("write", node->idname) == 0) {
                args = node->sibling->sibling;
                //write只有一个参数
                exp1 = args->child;
                translate_Exp(exp1, NONE, &op1);
                addIrWrite(&op1);

            } else if (strcmp("read", node->idname) == 0) {
                if (operator== ASSIGNOP) {
                    addIrRead(op);
                } else { //如果 仅调用函数而未赋值则会来到这里，如"read();",正确情况应该是t = read();
                    *op = *newTemp();
                    addIrRead(op);

                    //picnic("read函数一定是以 向某变量 赋值的形式调用的!");
                }
            } else {
                if (node->sibling->sibling->type == RP) {
                    assert(node->sibling->type == LP);
                } else {
                    args = node->sibling->sibling;
                    translate_Args(args);
                }
                if (operator== ASSIGNOP) {
                    //函数调用带赋值
                    addIrCall(op, node->idname);
                } else {
                    //LOG("%p",newTemp());
                    *op = *newTemp();
                    addIrCall(op, node->idname);
                }
            }
        }
        break;
    case Exp:
        assert(node->type == Exp);
        exp1 = node;
        re = node->sibling;
        exp2 = node->sibling->sibling;
        switch (re->type) {
        case ASSIGNOP:
            //translate_Exp(exp2,NONE)
            translate_Exp(exp1, NONE, &op1);

            if (op1.kind == OINFERENCE) {
                translate_Exp(exp2, NONE, &op2);
                if (!isOperandEqual(&op1, &op2)) {
                    addTwoInter(ASSIGNOP, &op1, &op2);
                }
            } else {
                translate_Exp(exp2, ASSIGNOP, &op1);
            }
            if (operator== ASSIGNOP) {
                //addTwoInter(ASSIGNOP, op, &op1);
            } else {
                //picnic("never come here!");
            }
            //addTwoInter(ASSIGNOP, &op1, &op2);
            break;
        case PLUS:
        case MINUS:
        case STAR:
        case DIV:
            translate_Exp(exp1, NONE, &op1);
            translate_Exp(exp2, NONE, &op2);
            if (operator== ASSIGNOP) {
                addThreeInter(re->type, op, &op1, &op2);
                //LOG("%d", op->kind);
            } else {
                *op = *newTemp(); //新建一个临时变量
                addThreeInter(re->type, op, &op1, &op2);
            }
            break;
        case AND:
        case OR:
        case RELOP:
            exp_type = exp_cond;
            break;
        case LB: //exp 是一个数组

            //translate_Exp(exp2, NONE, &op2);
            //SHOW("new");
            //printTree(exp, 1);
            array_address = newTemp(); //array_address 存储 取数组的地址

            // if (exp1->child->type != ID ) { //说明是对结构体域的访问，如 x.a[2] = 1;
            //     assert(exp1->child->type == Exp);
            //     //translate_Exp(exp1, ASSIGNOP, array_address);
            // } else {
            getArrayNameFromExp(exp1, arraytemp);
            strcpy(op1.u.varname, arraytemp);

            //判断数组是否是参数，若是则用*,否则用&
            op1.kind = isVaribleArg(op1.u.varname) ? OINFERENCE : OADDRESS;
            addTwoInter(ASSIGNOP, array_address, &op1);
            //}
            //op1.suffix = OADDRESS;

            array_address = getArrayAddressOffset(exp1, array_address);

            //取引用
            array_address->kind = OINFERENCE;
            if (operator== ASSIGNOP) {
                addTwoInter(ASSIGNOP, op, array_address);
            } else {
                //*op = *newTemp();
                *op = *array_address;
            }
            //addTwoInter(ASSIGNOP, op, array_address);

            free(array_address);
            free(arrayoffset);
            break;
        case DOT: //exp 是一个结构体变量的引用
            //TODO: 结构体引用
            struct_id = re->sibling;
            //LOG("%s", struct_id->nodename);
            //取变量首地址
            if (getStructVaribleName(exp1, arraytemp)) {
                strcpy(op1.u.varname, arraytemp);
                op1.kind = isVaribleArg(op1.u.varname) ? OINFERENCE : OADDRESS;

                //取变量偏移量
                op2.kind = OCONST;
                op2.u.value = getStructFieldOffset(arraytemp, struct_id->idname);

                //LOG("%d", getStructFieldOffset(arraytemp, struct_id->idname));
                //取引用
                //array_address->kind = OINFERENCE;
                if (operator== ASSIGNOP) {
                    addThreeInter(PLUS, op, &op1, &op2);
                } else {
                    array_address = newTemp();

                    array_address->kind = OVAR;
                    addThreeInter(PLUS, array_address, &op1, &op2);

                    *op = *array_address;
                }
            } else { //exp1 不是结构体而是复合型的

                if (operator== ASSIGNOP) {
                    translate_Exp(exp1, ASSIGNOP, &op1);
                    addTwoInter(ASSIGNOP, op, &op1);
                    //*op = op1;
                } else {
                    array_address = newTemp();
                    array_address->kind = OVAR;
                    translate_Exp(exp1, ASSIGNOP, array_address);
                    *op = *array_address;
                }
                //array_address = newTemp();
                //translate_Exp(exp1, ASSIGNOP, op);
            }

            break;
        }
        break;
    case LP:
        node = node->sibling;
        assert(node->type == Exp);
        translate_Exp(node, operator, op);
        break;
    case NOT:
        exp_type = exp_cond;
        //translate_Cond(exp);
        break;
    }
    if (exp_type == exp_cond) {
        //LOG("%d", operator);
        //LOG("%s","judgement");
        // LOG("%d %s",op->kind, op->u.varname);

        Label* label0 = newLabel();
        Label* label1 = newLabel();
        op1.kind = OCONST;
        op1.u.value = 1;
        op2.kind = OCONST;
        op2.u.value = 0;

        if (operator!= ASSIGNOP) {
            *op = *newTemp();
            op->u.value = 0;
        }
        // LOG("%d %s",op->kind, op->u.varname);
        translate_Cond(exp, label1, label0, NONE);

        addIrLable(label0);
        addTwoInter(ASSIGNOP, op, &op2);
        addIrLable(label1);

        addTwoInter(ASSIGNOP, op, &op1);
        //addIrLable(label0);
    } else {
        // LOG("%s","normal");
    }
}
int getArrayNameFromExp(TreeNode* exp, char* arrayname)
{ //这里分为exp 是id 和 struct 作用域两种情况
    assert(exp->type == Exp);
    for (; exp->type != ID; exp = exp->child) {
    }
    strcpy(arrayname, exp->idname);
    return 1;
}
//返回表示偏移量的Operand,
// Operand* getArrayAddressOffsetFromStruct(FieldList* field)
Operand* getArrayAddressOffset(TreeNode* exp, Operand* array_address)
{
    //printTree(exp, 1);
    assert(exp != NULL && exp->sibling->type == LB);
    Varkind kind;
    Vartype type;
    // LOG("%s", "let");
    getKindTypeOfArray(exp, array_address, &type, &kind);

    // LOG("%d", type.specifier);
    // assert(type.specifier == STRUCTVAR);
    // if (type.specifier == STRUCTVAR) {
    //     translate_Exp(exp, ASSIGNOP, array_address);
    // }

    // LOG("%d %d", kind.array.objnum, kind.array.dimension);

    Operand index, multiply;
    Operand* record = newTemp(); //只起到临时记录的作用
    //Operand objsize;

    //Operand* offset = newTemp(); //记录总偏移量
    TreeNode *exp1 = exp, *exp2 = NULL; //exp1->sibling->sibling;
    assert(exp1 != NULL);

    multiply.kind = OCONST;
    multiply.u.value = type.memory_size;
    ArrayList* arraylist = kind.array.array;
    assert(arraylist != NULL);
    // SHOW("a");
    int dimension = kind.array.dimension;
    //multiply *
    int optimize = 0; //能提前算的偏移量都先保存在这里，到时候一块儿加，
    for (int i = 1; i <= dimension; i++) {
        exp2 = exp1->sibling->sibling;
        // SHOW("<==");
        translate_Exp(exp2, NONE, &index);

        if (index.kind == OCONST) { //如果 a[Exp]中，Exp是一个常数，那么就可以现在就计算它了， 否则需要翻译成中间代码代码
            index.u.value *= multiply.u.value;
            optimize += index.u.value; //虽然 能提前计算某一维的偏移量，但还不急着加到初始数组地址上，等能提前算的部分都算完了再一块儿加
        } else {
            addThreeInter(STAR, record, &index, &multiply);
            addThreeInter(PLUS, array_address, array_address, record);
        }
        // multiply.u.value *= getRangeOfArray(arraylist, i, dimension);

        exp1 = exp1->child;
        // LOG("%s", exp1->nodename);
        // SHOW("==>");
    }
    if (optimize != 0) {
        index.kind = OCONST;
        index.u.value = optimize;
        addThreeInter(PLUS, array_address, array_address, &index);
    }
    // SHOW("b");
    return array_address;
}
int getRangeOfArray(ArrayList* arraylist, int no, int dimension)
{
    int t = dimension - no;
    for (int i = 0; i < t; i++) {
        arraylist = arraylist->tail;
    }
    return arraylist->array_size;
}
InterCodes* translate_Args(TreeNode* args)
{
    //TODO: 实参传递
    assert(args->type == Args);
    /** Args ->Exp COMMA Args
    *       | Exp 
    */
    TreeNode* exp = args->child;
    if (exp->sibling != NULL) {
        args = exp->sibling->sibling;
        translate_Args(args);
    }
    Operand op;
    translate_Exp(exp, NONE, &op);
    addIrArg(&op);
}
InterCodes* translate_Program(TreeNode* program)
{
    assert(program->type == Program);
    TreeNode* extdeflist = program->child;
    TreeNode* extdef = NULL;
    for (; extdeflist->child != NULL; extdeflist = extdeflist->child->sibling) {
        extdef = extdeflist->child;
        translate_ExtDef(extdef);
    }
}

InterCodes* translate_ParamDec(TreeNode* param)
{
    char temp[32];
    assert(param->type == ParamDec); //TODO:暂时只处理int;
    TreeNode* vardec = param->child->sibling;
    TreeNode* id = vardec->child;
    if (id->type != ID) { //说明是数组
        getArrayName(vardec, temp);
        addIrParam(temp);
    } else {
        assert(id->type == ID);
        addIrParam(id->idname);
    }
}
InterCodes* translate_FunDec(TreeNode* fundec)
{
    TreeNode* id = fundec->child;
    //printf("%s\n", id->idname);
    addIrFunction(id->idname);

    TreeNode* varlist = id->sibling->sibling;
    if (varlist->type != RP) { //说明是有形参的函数
        TreeNode* paramdec = varlist->child;
        assert(paramdec->type == ParamDec);

        TreeNode *spec = NULL, *vardec = NULL;
        //Vartype param_type;
        //LOG("%s", "start");
        for (; paramdec->sibling != NULL; varlist = paramdec->sibling->sibling, paramdec = varlist->child) {
            //核心是处理处理paramdec
            translate_ParamDec(paramdec);
        }
        translate_ParamDec(paramdec);
    }
}
InterCodes* translate_ExtDef(TreeNode* extdef)
{
    assert(extdef->type == ExtDef);
    TreeNode* node = extdef->child->sibling; //只考虑node 是 FunDec的情况;
    switch (node->type) {
    case FunDec:
        translate_FunDec(node);
        node = node->sibling;
        translate_CompSt(node);
        break;
    }
}
InterCodes* translate_CompSt(TreeNode* compst)
{
    assert(compst->type == CompSt);
    //deflist
    TreeNode* deflist = compst->child->sibling;
    TreeNode* def = deflist->child;
    for (; deflist->child != NULL; deflist = deflist->child->sibling) {
        def = deflist->child;
        translate_Def(def);
    }
    //stmtlist
    TreeNode* stmtlist = compst->child->sibling->sibling;
    TreeNode* stmt = stmtlist->child;
    for (; stmtlist->child != NULL; stmtlist = stmtlist->child->sibling) {
        stmt = stmtlist->child;
        translate_Stmt(stmt, NULL);
    }
}
InterCodes* translate_Def(TreeNode* def)
{
    assert(def->type == Def);
    //SHOW("来到");
    TreeNode* declist = def->child->sibling;
    TreeNode* dec = declist->child; //将vardec 置为表示语法 VarDec 的节点，
    //translate_Dec(dec);
    for (; dec->sibling != NULL; dec = dec->sibling->sibling->child) {
        translate_Dec(dec);
    }
    translate_Dec(dec);
}
InterCodes* translate_Dec(TreeNode* dec)
{
    assert(dec->type == Dec);
    TreeNode* vardec = dec->child;
    translate_VarDec(vardec);
}
InterCodes* translate_VarDec(TreeNode* vardec)
{
    assert(vardec->type == VarDec);
    TreeNode* id = vardec->child;
    OBJKIND objkind = (id->type == ID) ? VARIBLE : ARRAY;

    Vartype type;
    Operand op1;
    if (objkind == VARIBLE) {
        Vartype type = *findSymbolType(id->idname);
        if (type.specifier == STRUCTVAR) {
            op1.kind = OVAR;
            strcpy(op1.u.varname, id->idname);
            addIrDEC(&op1, type.memory_size);
        } else if (vardec->sibling != NULL && vardec->sibling->type == ASSIGNOP) { //可能存在向新定义变量赋值的情况

            TreeNode* exp = vardec->sibling->sibling;
            Operand left;
            left.kind = OVAR;
            strcpy(left.u.varname, id->idname);
            translate_Exp(exp, ASSIGNOP, &left);
        }

    } else if (objkind == ARRAY) {
        initialize_Array(vardec);
    }
}
void getArrayName(TreeNode* array, char* arrayname)
{
    assert(array->type == VarDec);
    TreeNode* id = array->child;
    Operand op;
    for (; id->type != ID; id = id->child) {
    }
    strcpy(arrayname, id->idname);
}
int initialize_Array(TreeNode* array)
{
    assert(array->type == VarDec);
    TreeNode* id = array->child;
    Operand op;
    for (; id->type != ID; id = id->child) {
    }
    Varkind* kind = findSymbolKind(id->idname);
    Vartype* type = findSymbolType(id->idname);

    assert(kind->objkind == ARRAY);
    ArrayList* arraylist = kind->array.array;
    int objnum = 1;
    for (int i = kind->array.dimension; i > 0; i--, arraylist = arraylist->tail) {
        objnum = objnum * arraylist->array_size;
    }
    objnum = objnum * type->memory_size;
    op.kind = OVAR;
    strcpy(op.u.varname, id->idname);

    addIrDEC(&op, objnum);

    return objnum;
    //LOG("%d", objnum);
    //SymbolNode* s = findSymbol(id->idname);
}
InterCodes* translate_Stmt(TreeNode* stmt, Label* if_end)
{
    assert(stmt->type == Stmt);
    //SHOW("cyd");
    TreeNode* node = stmt->child;
    TreeNode* ifelse = node;
    Operand op;
    switch (node->type) {
    case Exp:
        translate_Exp(node, NONE, &op);
        break;
    case RETURN:
        translate_Exp(node->sibling, NONE, &op);
        addOneInter(RETURN, &op);
        break;
    case CompSt:
        translate_CompSt(node);
        break;
    case IF:
        ifelse = ifelse->sibling->sibling->sibling->sibling->sibling;
        if (ifelse == NULL) {
            translate_IF(node, if_end);
        } else {
            translate_IF_ELSE(node, if_end);
        }
        break;
    case WHILE:
        translate_While(node, if_end);
        break;
    }
}
InterCodes* translate_While(TreeNode* whilestmt, Label* if_end)
{
    assert(whilestmt->type == WHILE);

    TreeNode* exp = whilestmt->sibling->sibling;
    TreeNode* stmt = exp->sibling->sibling;

    assert(stmt->type == Stmt);

    Label* label_startcond = newLabel();
    Label* label_endstmt = (if_end == NULL) ? newLabel() : if_end;

    addIrLable(label_startcond);

    translate_Cond(exp, label_endstmt, NULL, REVERSE);
    //while 中的stmt开始

    translate_Stmt(stmt, NULL);

    addIrGoto(label_startcond); //while中的语句体执行完毕后，再回到起始判断条件 判断，
    //结束
    if (if_end == NULL) {
        addIrLable(label_endstmt);
    }
}
InterCodes* translate_IF(TreeNode* ifstmt, Label* if_end)
{
    bool is_have_else = false;
    assert(ifstmt->type == IF);

    TreeNode* cond = ifstmt->sibling->sibling;
    TreeNode* stmt = cond->sibling->sibling; //stmt 表示if 语句中的 if (exp) stmt else stmt;
    // TreeNode* elsestmt = NULL;

    Label* label_true = newLabel();
    Label* label_false = (if_end == NULL) ? newLabel() : if_end;
    translate_Cond(cond, label_false, NULL, REVERSE);

    translate_Stmt(stmt, label_false);
    if (if_end == NULL) {
        addIrLable(label_false);
    }
}
InterCodes* translate_IF_ELSE(TreeNode* ifelsestmt, Label* if_end)
{
    assert(ifelsestmt->sibling->sibling->sibling->sibling->sibling != NULL);

    TreeNode* cond = ifelsestmt->sibling->sibling;
    TreeNode* stmt1 = cond->sibling->sibling;
    TreeNode* stmt2 = stmt1->sibling->sibling;

    Label* label_else = newLabel();

    Label* label_end = NULL;
    if (if_end == NULL) {
        label_end = newLabel();
    } else {
        label_end = if_end;
    }

    translate_Cond(cond, label_else, NULL, REVERSE);
    translate_Stmt(stmt1, label_end);
    addIrGoto(label_end);
    addIrLable(label_else);

    translate_Stmt(stmt2, label_end);
    if (if_end == NULL) {
        addIrLable(label_end);
    }
}
InterCodes* translate_Cond(TreeNode* exp, Label* label_true, Label* label_false, RELOP_TYPE type)
{ //总体可以分为有 比较符和 无比较符的情况
    assert(label_true != NULL);
    assert(exp != NULL && exp->type == Exp);
    enum { exp_relop,
        exp_relcal,
        exp_number,
        empty } cond_type;
    cond_type = empty;

    TreeNode* exp1 = NULL;
    TreeNode* exp2 = NULL;
    TreeNode* relop = NULL;

    assert(exp->type == Exp); // 来到这里，说明Exp的产生式只能是: Exp -> Exp relop Exp | NOT Exp | 等情况

    Operand op1, op2;
    COMPARISON comparison;
    exp1 = exp->child;
    //判断 cond 中exp的类型
    switch (exp1->type) {
    case Exp:
        switch (exp1->sibling->type) {
        case AND:
        case OR:
            cond_type = exp_relcal;
            relop = exp1->sibling;
            exp2 = relop->sibling;
            break;
        case MINUS:
        case PLUS:
        case STAR:
        case DIV:
            exp2 = exp1->sibling->sibling;
            cond_type = exp_number;
            break;
        case RELOP:
            relop = exp1->sibling;
            cond_type = exp_relop;
            exp2 = exp1->sibling->sibling;
            break;
        }
        break;
    case FLOAT:
    case ID:
    case INT:
        cond_type = exp_number;
        break;
    case LP: //(exp)形式，直接继续调用translate_Cond即可
        exp1 = exp1->sibling;
        translate_Cond(exp1, label_true, label_false, NONE);
        break;
    case NOT: //if(!exp)形式，继续调用，只是label_true 和 label_false对调
        exp1 = exp1->sibling;
        translate_Cond(exp1, label_false, label_true, NONE);
        break;
    }
    //对三种 cond 类型分别处理
    Label* label = newLabel(); //为处理 关系运算符 and or 准备的
    switch (cond_type) {
    case exp_number: //cond 是一个数或表达式的情形 如 if( 1 + 1 )等等
        translate_Exp(exp, NONE, &op1);
        op2.kind = OCONST;
        op2.u.value = 0;

        comparison = NE;
        comparison = (type == REVERSE) ? reverse_comp(comparison) : comparison;
        addIrIf(IF, &op1, &op2, comparison, label_true);
        if (label_false != NULL) { //为了优化代码而设定的
            addIrGoto(label_false);
        }
        break;
    case exp_relop: //cond 由关系判别符组成，形如 if(exp > exp) 等等
        //LOG("%s", relop->nodename);
        assert(relop->type == RELOP);
        //exp1 relop exp2

        Operand* op1 = newTemp();
        Operand* op2 = newTemp();
        translate_Exp(exp1, NONE, op1);
        translate_Exp(exp2, NONE, op2);

        comparison = relop->comparison;
        comparison = (type == REVERSE) ? reverse_comp(comparison) : comparison;
        addIrIf(IF, op1, op2, comparison, label_true);

        if (label_false != NULL) {
            addIrGoto(label_false);
        }
        break;
    case exp_relcal: //cond 由一系列关系运算组成 形如 if( 2 && 1) 等等
        switch (relop->type) {
        case AND:
            translate_Cond(exp1, label, label_false, NONE);
            addIrLable(label);
            translate_Cond(exp2, label_true, label_false, NONE);
            break;
        case OR:
            translate_Cond(exp1, label_true, label, NONE);
            addIrLable(label);
            translate_Cond(exp2, label_true, label_false, NONE);
            break;
        }
        break;
    case empty:
        break;
    }
}
void startGenerateIr(TreeNode* tree)
{
    translate_Program(tree);
}
void testrun()
{
    Operand op1, op2;
    Label label;
    op1.kind = OCONST;
    op1.u.value = 2;
    op2.kind = OCONST;
    op2.u.value = 3;
    strcpy(label.labelname, "label1");
    // addIrIf(IF, &op1, &op2, GEZ, &label);
}
COMPARISON reverse_comp(COMPARISON relop)
{
    switch (relop) {
    case EQUAL:
        relop = NE;
        break;
    case NE:
        relop = EQUAL;
        break;
    case GZ:
        relop = SEZ;
        break;
    case SZ:
        relop = GEZ;
        break;
    case GEZ:
        relop = SZ;
        break;
    case SEZ:
        relop = GZ;
        break;
    }
    return relop;
}
int calculateDomainAddress(Vartype* type, char* domain_name)
{
    Vartype structtype;
    findStructNode(type->struct_id, &structtype);
    int memorize = 0;

    return memorize;
}
int getStructVaribleName(TreeNode* exp, char* name)
{ //这里假设exp下仅包含id;
    assert(exp->type == Exp);
    TreeNode* id = exp->child;
    if (id->type == ID) {
        strcpy(name, id->idname);
        return 1;
    } else {
        return 0;
    }
}
int getStructFieldOffset(char* variblename, char* fieldname)
{
    assert(variblename != NULL && fieldname != NULL);
    int offset = 0;
    Vartype type = *findSymbolType(variblename);
    FieldList* field = type.structure;

    do {

        switch (field->kind.objkind) {
        case VARIBLE:
            offset += field->type.memory_size;
            //SHOW("1");
            break;
        case ARRAY:
            offset += field->type.memory_size * (field->kind.array.objnum);
            //LOG("%d %d %d", field->type.memory_size,field->kind.array.objnum,offset);
            //SHOW("2");
            break;
        default:
            picnic("never come here");
        }

        if (strcmp(field->name, fieldname) == 0) {
            break;
        }
        field = field->tail;
    } while (field != NULL); // && strcmp(field->name, fieldname) != 0
    //LOG("吼啊 %d", offset);
    // for (; field != NULL && strcmp(field->name, fieldname) != 0; field = field->tail) { //&& strcmp(field->name,fieldname) != 0
    //     //LOG("%s", field->name);
    //     assert(field->kind.objkind == VARIBLE);
    //     offset += field->type.memory_size;
    // }

    return type.memory_size - offset;
}
int getKindTypeOfArray(TreeNode* exp, Operand* address, Vartype* type, Varkind* kind)
{
    assert(exp->type == Exp);
    TreeNode* id = exp;
    TreeNode* fieldname = NULL;
    TreeNode* struct_dot_field = NULL;
    Vartype localtype;
    Varkind localkind;
    for (; id != NULL; id = id->child) {
        if (id->sibling != NULL && id->sibling->type == DOT) { //说明此时id是结构体了
            // SHOW("结构体来喽");
            fieldname = id->sibling->sibling;
            assert(fieldname->type == ID);
            // SHOW("pass");
            //id = id->child;
            dealExp(id, &localtype, &localkind);
            //findStructNode(id->idname, &localtype);
            getStructDomain(fieldname->idname, localtype.structure, type, kind);
            translate_Exp(struct_dot_field, ASSIGNOP, address);
            // SHOW("end");
            break;
        } else if (id->type == ID) {
            *type = *findSymbolType(id->idname);
            *kind = *findSymbolKind(id->idname);
            break;
        }
        struct_dot_field = id;
    }
}