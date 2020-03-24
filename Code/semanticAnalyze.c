#include "semanticAnalyze.h"
#include "symbolTableManage.h"
#include "debug.h"
#include <string.h>
static int dynamic_deep = 0;
static char structname[32];
static char *error_type_content[] = {
    /* 0 */ "Undefined varible",
    "Undefined function",
    "Redefined varible",
    "Redefined function",
    "Type mismatched for assignment",
    /* 5 */ "The left-hand side of an assignment must be a variable",
    "Type mismatched for operands",
    "Type mismatched for return",
    "***", //这是用于打印函数错误的，因为它又有两个参数，所以就不用这个数组了
    "is not an array",
    /* 10 */ "is not a function",
    "is not an integer",
    "Illegal use of", //变量不是结构体，所以不能用访问符 .
    "Non-existent field",
    "Redefined field",
    /* 15 */ "Duplicated name",
    "Undefined structure"};
static Vartype return_type; /*用于判断一个CompSt中的返回值类型是否域函数类型匹配，
                                * 在产生式Specifier FunDec CompSt 中的Specifier里
                                * 也就是在处理ExtDef时 被赋值 
                                * */
int analyzeTree(TreeNode *node)
{
    TreeNode* extdeflist = NULL;
    TreeNode* extdef = NULL;
    switch (node->type) { //默认情况下，调用这个函数，传入的节点都是Program,
    case Program:
        extdeflist = node->child;
        for (; extdeflist->child != NULL;extdeflist = extdeflist->child->sibling){
            extdef = extdeflist->child;
            dealExtDef(extdef);
            //printSymTab(node);
            //printTree(node, 0);
        }
        break;
    default:
        picnic("never come here!");
    }
}
int dealExtDef(TreeNode *node)
{
    //这些节点的子节点将会定义变量或函数
    assert(node->type == ExtDef);
    TreeNode *spec = node->child;
    TreeNode *extdeclist = spec->sibling;
    TreeNode *fundec = NULL, *id = NULL, *vardec = NULL;
    TreeNode *compst = NULL;
    Vartype type;
    //Varkind kind;
    dealVartype(spec, &type);
    switch (extdeclist->type)
    {
    case ExtDecList:
        //assert(extdeclist->type == ExtDecList);
        vardec = extdeclist->child; //将vardec 置为表示语法 VarDec 的节点，
        dealVarDec(vardec, &type, 0);
        while (vardec->sibling != NULL)
        { //递归的将ExtDecList中的所有变量 都添加到符号表中
            extdeclist = vardec->sibling->sibling;
            vardec = extdeclist->child;
            dealVarDec(vardec, &type, 0); //添加此变量，
        }
        break;
    case FunDec:
        fundec = extdeclist; //因为 文法 中 这两个非终结符是平级的
        compst = fundec->sibling;
        id = fundec->child;
        Varkind *kind = malloc(sizeof(Varkind));
        //初始化 kind
        kind->function.param = NULL;
        
        //kind->array.array = NULL;

        char funct_name[32];
        dealFunction(fundec, kind, funct_name);

        //findSymbol(funct_name);
        if (isPresence(funct_name))
        {
            printError(FUNC_REDECLARATION, id->lineno, id->idname);
        }
        else
        {
            addFunction(funct_name, &type, kind);
        }
        if (compst->type == CompSt)
        {
            return_type = type; /* 在处理CompSt前向 return_type 写入
                                    * 此函数的返回类型,方便在return 时判断
                                    * 返回类型是否合法 */
            dealCompSt(fundec->sibling, 1);
            kind->function.isdefined = 1;
        }
        else if(compst->type == SEMI){     //函数申明的情况
            LOG("%s", "有一个申明");
            kind->function.isdefined = 0;
        }
        free(kind);
        break;
    default:
        break;
    }
    return 1;
}
int dealVartype(TreeNode *node, Vartype *type)
{
    if (NULL == type)
    {
        picnic("the type is null");
        exit(-1);
    }
    assert(node->type == Specifier);
    
    node = node->child;
    type->specifier = node->specifier;
    if(node->type == TYPE){
        type->memory_size = 4;
        //LOG("%d", type->memory_size);
    }
    //type->memory_size = 4;
    if (node->type == StructSpecifier)
    { //单独处理struct类型
        node = node->child;
        assert(node->type == STRUCT);
        node = node->sibling; //此时 node 是 OptTag 或者 Tag
        if (node->type == OptTag)
        {
            assert(node->type == OptTag);
            if (node->child == NULL)
            {
                //说明这是个空产生式，即 struct 后没有ID,直接是{...}
                strcpy(type->struct_id, "\0");
            }
            else
            {
                TreeNode *deflist = node->sibling->sibling;

                assert(type->specifier == STRUCTVAR);
                node = node->child;
                assert(node->type == ID);
                strcpy(type->struct_id, node->idname);
                if (findStructNode(node->idname, type) > 0)
                {
                    //struct 表中已经存在这个 struct了
                    printError3(DUPLICATED_NAME, node->lineno, node->idname);
                }
                else
                {
                    type->structure = NULL; //多次来到这个函数需要初始化type;
                    type->memory_size = 0;
                    addStructDomain(deflist, type);
                    //LOG("%d", type->memory_size);
                    addStructNode(type);
                }
            }
        }
        else if (node->type == Tag)
        {
            node = node->child;
            int ishave = findStructNode(node->idname, type);

            //LOG("%d", type->memory_size);
            if (ishave < 0) {
                printError3(UNDEFINED_STRUCTURE, node->lineno, node->idname);
                //picnic("未定义的 struct ");
            }
        }
    }
    return 1;
}
int dealFunction(TreeNode *node, Varkind *kind, char *funct_name)
{
    kind->objkind = FUNCTION;
    //like dealArray, desolve params;
    TreeNode *id = node->child;
    strcpy(funct_name, id->idname);
    TreeNode *varlist = id->sibling;
    if (varlist->sibling->type == VarList)
    {
        //LOG("%s", "函数有形参");
        //说明函数中有形参
        varlist = varlist->sibling;
        //FieldList *param = kind->function.param;
        TreeNode *paramdec = varlist->child;
        assert(paramdec->type == ParamDec);

        TreeNode *spec = NULL, *vardec = NULL;
        Vartype param_type;
        //LOG("%s", "start");
        for (; paramdec->sibling != NULL; varlist = paramdec->sibling->sibling, paramdec = varlist->child)
        {
            //核心是处理处理paramdec
            //LOG("%s", "cyd");
            spec = paramdec->child;
            vardec = spec->sibling;
            dealVartype(spec, &param_type);
            //以下这个函数 将获得一个参数的变量，并将其id与刚刚获得的param_type一起放进kind中，
            //kind是用于返回的
            dealFunctionVarDec(vardec, &param_type, kind);
        }
        //LOG("%s", "cyd");
        spec = paramdec->child;
        vardec = spec->sibling;
        dealVartype(spec, &param_type);
        //以下这个函数 将获得一个参数的变量，并将其id与刚刚获得的param_type一起放进kind中，
        //kind是用于返回的
        dealFunctionVarDec(vardec, &param_type, kind);
    }
    else
    {
        kind->function.param = NULL;
        kind->function.param_num = 0;
        //此函数无形参
    }
    // if (isPresence(id->idname))
    // {
    //     printError(FUNC_REDECLARATION, id->lineno, id->idname);
    // }
    // else
    // {
    //     addFunction(id->idname, &type);
    // }
    return 1;
}
//node 是一个VarDec ，这个函数返回数组名arrayname,并将其中的属性加入到kind中，
int dealArray(TreeNode *node, Varkind *kind, char *arrayname)
{
    assert(node->type == VarDec);
    int dimension = 1;
    int objnum = 1;
    ArrayList* array = malloc(sizeof(ArrayList));
    array->tail = NULL;
    array->array_size = node->sibling->sibling->int_num;
    kind->objkind = ARRAY;
    kind->array.array = array;
    objnum *= array->array_size;
    //LOG("%d", array->array_size);

    for (node = node->child; node->type != ID; node = node->child)
    {
        array = malloc(sizeof(ArrayList));
        array->tail = NULL;
        array->array_size = node->sibling->sibling->int_num;

        array->tail = kind->array.array;
        kind->array.array = array;
        //LOG("%s","cyd");
        //array->tail = malloc(sizeof(ArrayList));
        objnum *= array->array_size;
        dimension += 1;
    }
    //此时 node 的属性是 ID;
    kind->array.dimension = dimension;
    kind->objkind = ARRAY;
    strcpy(arrayname, node->idname);

    kind->array.objnum = objnum;
    
    return objnum;
}
int dealCompSt(TreeNode *compst, int deep)
{
    Vartype type;
    //deflist
    TreeNode *deflist = compst->child->sibling;
    TreeNode *def = deflist->child;
    for (; deflist->child != NULL; deflist = deflist->child->sibling)
    {
        def = deflist->child;
        dealVartype(def->child, &type);
        dealDecList(def->child->sibling, &type, 1);
    }
    //stmtlist
    TreeNode *stmtlist = compst->child->sibling->sibling;

    assert(stmtlist->type == StmtList);
    if (stmtlist->child == NULL)
    {
        return 1; //说明此 stmtlist的产生式是空
    }
    TreeNode *stmt = stmtlist->child;
    //LOG("%s", stmtlist->nodename);
    for (; stmtlist->child != NULL; stmtlist = stmt->sibling, stmt = stmtlist->child)
    {
        dealStmt(stmt, deep);
    } /**/
    return 1;
}
int dealStmt(TreeNode *node, int deep)
{
    assert(node->type == Stmt);
    TreeNode *next = node->child;
    Vartype type;
    Varkind kind;

    TreeNode *exp = NULL;
    switch (next->type)
    {
    case Exp:
        dealExp(next, &type, &kind);
        break;
    case CompSt:
        dealCompSt(next, deep + 1);
        break;
    case RETURN:
        exp = next->sibling;
        dealExp(exp, &type, &kind);
        if (!isTypeEqual(&return_type, &type))
        {
            printError1(MISMATCHED_FOR_RETURN, exp->lineno);
        }
        break;
    case WHILE: /*//这其实可以和下面的IF的条件合并
        next = next->sibling->sibling;
        dealExp(next, &type, &kind);
        next = next->sibling->sibling;
        dealStmt(next, deep);
        break;*/
    case IF:
        next = next->sibling->sibling;
        dealExp(next, &type, &kind);
        next = next->sibling->sibling;
        dealStmt(next, deep);
        if (next->sibling != NULL) //WHILE的情况到这儿 肯定不会通过
        {                          //有 ELSE Stmt 的情况
            dealStmt(next->sibling->sibling, deep);
        }
        break;
    }
    return 1;
}
int dealFunctionVarDec(TreeNode *node, Vartype *param_type, Varkind *kind)
{
    //类似于 dealStructVarDec
    assert(node->type == VarDec);
    node = node->child;
    OBJKIND objkind = (node->type == ID) ? VARIBLE : ARRAY;

    FieldList *field = malloc(sizeof(FieldList));
    field->type = *param_type;
    field->tail = NULL;

    //lab3:
    SymbolNode* is_arg = NULL;

    if (objkind == VARIBLE) {
        field->kind.objkind = VARIBLE;
        strcpy(field->name, node->idname);

        is_arg = addLocalVarible(field->name, param_type, 1);
        is_arg->varible_is_arg = 1;  //标记此变量是函数参数
    } else { //此变量是数组
        Varkind param_kind;
        char arrayname[32];
        dealArray(node, &param_kind, arrayname);

        field->kind.objkind = ARRAY;
        strcpy(field->name, arrayname);

        field->kind = param_kind;

        is_arg = addArray(arrayname, param_type, &field->kind, 1);
        is_arg->varible_is_arg = 1;  //标记此数组是函数参数
    }
    //加到kind->function.param的末尾
    FieldList *t = kind->function.param;
    if (t == NULL)
    {
        kind->function.param = field;
    }
    else
    {
        for (; t->tail != NULL; t = t->tail)
            ;
        field->tail = kind->function.param;
        kind->function.param = field;
        t->tail = field;
    }

    return 1;
}
int dealStructVarDec(TreeNode *node, Vartype *domain_type, Vartype *global_type)
{
    //用域struct 中的域 新申明的变量还需要定义其类型,global_type是这个struct要服务的type;
    node = node->child;
    OBJKIND objkind = (node->type == ID) ? VARIBLE : ARRAY;

    FieldList *s = malloc(sizeof(FieldList));
    s->tail = NULL;
    if (objkind == VARIBLE)
    {
        if (findStructDomain(node->idname, global_type->structure))
        {
            //检查域中是否已有 同名的域名
            printError3(REDEFINED_FIELD, node->lineno, node->idname);
            free(s);
            return -1;
        }
        else
        {
            s->type = *domain_type;
            strcpy(s->name, node->idname);
            s->kind.objkind = VARIBLE;

            global_type->memory_size += domain_type->memory_size;
            //LOG("%d,%d", global_type->memory_size,domain_type->memory_size);
        }
    }
    else
    { //此变量是数组
        Varkind kind;
        char arrayname[32];
        int objnum = dealArray(node, &kind, arrayname);
        //LOG("%s", arrayname);
        if (findStructDomain(arrayname, global_type->structure))
        {
            //检查域中是否已有 同名的域名
            printError3(REDEFINED_FIELD, node->lineno, arrayname);
            free(s);
            return -1;
        }
        else
        {
            s->type = *domain_type;
            strcpy(s->name, arrayname);
            s->kind = kind;
            //s->kind.objkind = ARRAY;

            global_type->memory_size += objnum * (domain_type->memory_size);
            //LOG("%d %d",kind.array.objnum, objnum);
        }
    }
    //现在开始向global_type中加入这一个项，并且新项在前
    FieldList *t = global_type->structure;
    s->tail = t;
    global_type->structure = s;
    return 1;
}
int dealVarDec(TreeNode *node, Vartype *type, int deep)
{
    //新申明的变量还需要定义其类型
    node = node->child;
    OBJKIND objkind = (node->type == ID) ? VARIBLE : ARRAY;

    if (objkind == VARIBLE)
    {
        if (isPresence(node->idname))
        {
            printError(VARBILE_REDECLARATION, node->lineno, node->idname);
        }
        else
        {
            addLocalVarible(node->idname, type, deep);
        }
    }
    else
    { //此变量是数组
        Varkind kind;
        char arrayname[32];
        dealArray(node, &kind, arrayname);
        if (isPresence(arrayname))
        {
            printError(VARBILE_REDECLARATION, node->lineno, arrayname);
        }
        else
        {
            addArray(arrayname, type, &kind, deep);
        }
    }
    return 1;
}
bool isExpLeftValue(TreeNode *node)
{
    TreeNode *exp = node;
    assert(exp->type == Exp);
    if (exp->child->type == ID && exp->child->sibling == NULL)
    {
        return true; //Exp 是一个变量
    }
    else if (exp->child->type == Exp)
    {
        exp = exp->child; //Exp 是一个 struct 成员 或 数组
        return exp->sibling->type == LB || exp->sibling->type == DOT;
    }
    return false;
}
int dealExp(TreeNode *exp_node, Vartype *type, Varkind *kind)
{ //exp_node:指向节点树中类型为Exp节点的指针，type kind 用于向上返回这个exp节点所示变量的类型和种类，
    //TODO: 保证每次调用此函数都会返回 type 和 kind,无论是否发生错误
    assert(exp_node != NULL);
    assert(exp_node->type == Exp);

    TreeNode *node = exp_node->child;

    Vartype exp1_type;
    Vartype exp2_type;

    Varkind exp1_kind;
    Varkind exp2_kind;
    //Exp 只是一个变量
    switch (node->type)
    {
    case MINUS:
        node = node->sibling;
        dealExp(node, type, kind);
        break;
    case INT:
        type->specifier = INTVAR;
        kind->objkind = VARIBLE;
        break;
    case FLOAT:
        type->specifier = FLOATVAR;
        kind->objkind = VARIBLE;
        break;
    case LP:
        dealExp(node->sibling, type, kind);
        break;
    case ID:
        if (ID == node->type && node->sibling == NULL)
        { //Exp 是一个变量
            if (!isPresence(node->idname))
            {
                printError(USING_VARIBLE_BUT_NOT_DEFINED, node->lineno, node->idname);
                type->specifier = ANYVAR;
                kind->objkind = ANYKIND;
            }
            else
            {
                *type = *findSymbolType(node->idname);
                *kind = *findSymbolKind(node->idname);
            }
        }
        else if (ID == node->type && LP == node->sibling->type)
        { //Exp 是一个函数
            kind->objkind = VARIBLE;   //形如 id(),那么它就代表一个变量而非函数了

            if (!isPresence(node->idname))
            {
                printError(USING_FUNC_BUT_NOT_DEFINED, node->lineno, node->idname);
                type->specifier = ANYVAR;
                kind->objkind = ANYKIND;
            }
            else if (findSymbolKind(node->idname)->objkind != FUNCTION)
            {
                printf("\033[22;35mError type %d at Line %d: ", NOT_AN_FUNCTION, node->lineno);
                printf("\"%s\" is not a function\n\033[0m", node->idname);
            }
            else
            {
                //kind->objkind = FUNCTION;
                SymbolNode *funct = findSymbol(node->idname);
                Varkind *funct_kind = &funct->kind;
                FieldList *funct_param = funct_kind->function.param;
                *type = *findSymbolType(node->idname);

                TreeNode *args = node->sibling->sibling;

                if (!isParamArguEqual(funct_param, args))
                {
                    //LOG("%s","参数类型不匹配")
                    printf("\033[22;35mError type %d at Line %d: ", FUNCTION_NOT_APPLICABLE, node->sibling->lineno);
                    printf("function \"");
                    printFunctionParam(node->idname, funct_kind, type);
                    printf("\" is not applicate for args \"(");
                    printArgs(args);

                    printf(")\".\033[0m\n");
                }
            }
        }
        break;
    case Exp:
        dealExp(node, &exp1_type, &exp1_kind);
        Vartype int_type;
        bool isassign = false;
        char *domain_object_name = NULL;
        TreeNode *t = node->sibling;
        switch (node->sibling->type)
        {
        case DOT:
            domain_object_name = t->sibling->idname; //t->sibling的type是 ID;
            if (exp1_type.specifier != STRUCTVAR)
            {
                printf("\033[22;35mError type %d at Line %d: ", ILLEGAL_USE_OF_DOT, node->lineno);
                printf("\"");
                printExp(node);
                printf("\" is not a struct varible, illegal use of \".\"\n\033[0m");
            }
            else if (!findStructDomain(domain_object_name, exp1_type.structure))
            {
                printError(NON_EXISTENT_FIELD, t->sibling->lineno, domain_object_name);
                type->specifier = ANYVAR;
                kind->objkind = ANYKIND;
            }
            else
            {
                getStructDomainTypeKind(domain_object_name, &exp1_type, type, kind);
            }
            break;
        case LB:
            dealExp(t->sibling, &int_type, &exp2_kind);
            if (int_type.specifier != INTVAR)
            {
                printf("\033[22;35mError type %d at Line %d: ", NOT_AN_INT_FOR_ARRAY_INDEX, t->sibling->lineno);
                printf("\"");
                printExp(t->sibling);
                printf("\" is not an integer\n\033[0m");
            }
            if (exp1_kind.objkind != ARRAY)
            {
                printf("\033[22;35mError type %d at Line %d: ", NOT_AN_ARRAY, node->lineno);
                printf("\"");
                printExp(node);
                printf("\" is not an array\n\033[0m");
                //printError3(NOT_AN_ARRAY, node->lineno, node->idname);
            }
            //LOG("%d", exp1_kind.array.dimension);
            kind->array.dimension = exp1_kind.array.dimension - 1; //准确返回数组类型，如int a[2][3],那么a[1]仍然是数组，只是减少一维,
            kind->objkind = (kind->array.dimension == 0) ? VARIBLE : ARRAY;

            *type = exp1_type;
            break;
        default: //如果到这里意味着 Exp的产生式 Exp op Exp;
            if (t->type == ASSIGNOP)
            { //这个可以和后面的 处理产生式 Exp op Exp一起处理
                isassign = true;
                if (!isExpLeftValue(node))
                {
                    printError1(LEFT_HAND_MUST_BE_VARIBLE, node->lineno);
                }
            }
            TreeNode *exp2 = t->sibling;
            dealExp(exp2, &exp2_type, &exp2_kind);
            if (!isTypeKindEqual(&exp1_type, &exp1_kind, &exp2_type, &exp2_kind))
            {
                if (isassign)
                {
                    printError1(MISMATCHED_FOR_ASSIGNMENT, exp2->lineno);
                }
                else
                {
                    printError1(MISMATCHED_FOR_OPERANDS, exp2->lineno);
                }
            }
            *type = exp1_type;
            *kind = exp1_kind;
            break;
        }
        break;
    }
    return 1;
}
bool isParamArguEqual(FieldList *param, TreeNode *args)
{
    if (args->type != Args && args->type == RP)
    {
        //那么说明一个函数调用 没有实参
        if (param != NULL)
        { //而这个函数是有形参的，所以不匹配
            return false;
        }
        else
        {
            return true;
        }
    }
    else
    {
        assert(args->type == Args);
        if (param == NULL)
        { //本无形参 但 有实参，返回不匹配
            return false;
        }
    }
    TreeNode *exp = args->child;
    Vartype *argu_type = malloc(sizeof(Vartype));
    Varkind *argu_kind = malloc(sizeof(Varkind));
    bool flag = true;

    dealExp(exp, argu_type, argu_kind);
    flag = flag && isTypeKindEqual(argu_type, argu_kind, &param->type, &param->kind);

    if (param->tail != NULL && exp->sibling != NULL)
        do
        {
            param = param->tail;

            exp = exp->sibling->sibling->child;

            dealExp(exp, argu_type, argu_kind);
            flag = flag && isTypeKindEqual(argu_type, argu_kind, &param->type, &param->kind);
        } while (param->tail != NULL && exp->sibling != NULL);

    flag = flag && (param->tail == NULL) && (exp->sibling == NULL);
    //我开始羡慕自己了，怎么能写这么妙的代码，太精妙了
    free(argu_kind);
    free(argu_type);
    return flag;
}
int dealVarDecAssign(TreeNode *node, Vartype *type, Varkind *kind, int deep)
{
    node = node->child;
    OBJKIND objkind = (node->type == ID) ? VARIBLE : ARRAY;
    if (kind->objkind != objkind)
    {
        //LOG("%s", "这里是带赋值的初始化")
        printError1(MISMATCHED_FOR_ASSIGNMENT, node->lineno);
        return -1;
    }
    if (objkind == VARIBLE)
    {
        if (isPresence(node->idname))
        {
            printError(VARBILE_REDECLARATION, node->lineno, node->idname);
        }
        else
        {
            addLocalVarible(node->idname, type, deep);
        }
    }
    else
    { //此变量是数组
        Varkind kind;
        char arrayname[32];
        dealArray(node, &kind, arrayname);
        if (isPresence(arrayname))
        {
            printError(VARBILE_REDECLARATION, node->lineno, arrayname);
        }
        else
        {
            addArray(arrayname, type, &kind, deep);
        }
    }
    return 1;
}
int dealDecList(TreeNode *declist, Vartype *type, int deep)
{
    //TODO:将文法变量DecList中的所有变量添加到符号表，参数包含 变量的类型和作用域的深度
    TreeNode *dec = declist->child; //将vardec 置为表示语法 VarDec 的节点，
    assert(dec->type == Dec);

    Vartype localtype;
    Varkind localkind;
    TreeNode *vardec = NULL;

    dec = declist->child;

    vardec = dec->child;
    if (vardec->sibling != NULL)
    {
        //若进入此函数，说明变量有赋值部分，即 Dec -> VarDec ASSIGNOP Exp
        assert(vardec->sibling->type == ASSIGNOP);
        TreeNode *exp = vardec->sibling->sibling;
        dealExp(exp, &localtype, &localkind);
        if (!isTypeEqual(type, &localtype))
        { //赋值出现类型不等价
            //LOG("%s", "yes");
            //assert(type->specifier == INTVAR);
            //assert(localtype.specifier == INTVAR);
            LOG("%d = %d", type->specifier, localtype.specifier);
            printError(MISMATCHED_FOR_ASSIGNMENT, exp->lineno, "");
        }
        dealVarDecAssign(vardec, type, &localkind, deep);
    }
    else
    {
        dealVarDec(vardec, type, deep);
    }
    while (dec->sibling != NULL)
    { //递归的将ExtDecList中的所有变量 都添加到符号表中
        declist = dec->sibling->sibling;
        dec = declist->child;

        vardec = dec->child;
        if (vardec->sibling != NULL)
        {
            //若进入此函数，说明变量有赋值部分，即 Dec -> VarDec ASSIGNOP Exp
            assert(vardec->sibling->type == ASSIGNOP);
            TreeNode *exp = vardec->sibling->sibling;
            dealExp(exp, &localtype, &localkind);
            if (!isTypeEqual(type, &localtype))
            { //赋值出现类型不等价
                //LOG("%s", "yes");
                //assert(type->specifier == INTVAR);
                //assert(localtype.specifier == INTVAR);
                printError(MISMATCHED_FOR_ASSIGNMENT, exp->lineno, "");
            }
            dealVarDecAssign(vardec, type, &localkind, deep);
        }
        else
        {
            dealVarDec(vardec, type, deep);
        }
    }
    return 1;
}
int getStructDomainTypeKind(char *domain_name, Vartype *struct_type, Vartype *type, Varkind *kind)
{
    //findStructNode()
    FieldList *field = struct_type->structure;
    for (; field != NULL; field = field->tail)
    {
        if (strcmp(domain_name, field->name) == 0)
        {
            *type = field->type;
            *kind = field->kind;
            //return true;
        }
    }
    type = NULL;
    kind = NULL;
}
bool findStructDomain(char *name, FieldList *field)
{
    for (; field != NULL; field = field->tail)
    {
        if (strcmp(name, field->name) == 0)
        {
            return true;
        }
    }
    return false;
}
bool getStructDomain(char *fieldname, FieldList *field,Vartype* type,Varkind* kind)
{
    for (; field != NULL; field = field->tail)
    {
        if (strcmp(fieldname, field->name) == 0)
        {
            *type = field->type;
            *kind = field->kind;
            return true;
        }
    }
    return false;
}
int addStructDomain(TreeNode *node, Vartype *type) //其实是处理DefList的一个用于STRUCT中的域的特例
{
    //添加struct 中域
    assert(node->type == DefList);

    Vartype *domain_type = malloc(sizeof(Vartype));
    TreeNode *deflist = node;
    if (strcmp(deflist->nodename, "empty") == 0)
    {
        type->structure = NULL;
        //此函数内没有新申明变量
    }
    TreeNode *def = deflist->child;
    for (; def != NULL; def = def->sibling->child)
    {
        assert(def->type == Def);
        //类型赋予
        //specifier = getSpecifier(def->child);
        dealVartype(def->child, domain_type);
        //dealDecList(def->child->sibling, &type, 1);
        TreeNode *declist = def->child->sibling;

        assert(declist->type == DecList);

        TreeNode *dec = declist->child; //将vardec 置为表示语法 VarDec 的节点，
        assert(dec->type == Dec);

        Vartype localtype;
        Varkind localkind;
        TreeNode *vardec = dec->child;
        if (vardec->sibling != NULL)
        {
            //若进入此函数，说明变量有赋值部分，即 Dec -> VarDec ASSIGNOP Exp
            assert(vardec->sibling->type == ASSIGNOP);
            TreeNode *exp = vardec->sibling->sibling;
            dealExp(exp, &localtype, &localkind);
            if (!isTypeEqual(domain_type, &localtype))
            { //赋值出现类型不等价
                printError(MISMATCHED_FOR_ASSIGNMENT, exp->lineno, "");
            }
        }
        //dealVarDec(dec->child, type, deep);
        //type->memory_size = 0;
        dealStructVarDec(dec->child, domain_type, type);

        while (dec->sibling != NULL)
        { //递归的将ExtDecList中的所有变量 都添加到符号表中
            declist = dec->sibling->sibling;
            dec = declist->child;
            dealStructVarDec(dec->child, domain_type, type);
        }
    }
    //检测 处理结果;
    FieldList *s = type->structure;
    //释放这个暂时的空间
    free(domain_type);
    return 1;
}
VARSpecifier getSpecifier(TreeNode *node) //接受指向 Specifier 节点的指针
{                                         //此函数用于解析 Specifier 非终结符，已获得其中的类型，然后将这个属性赋予其后的变量等；
    assert(strcmp(node->nodename, "Specifier") == 0);
    node = node->child;
    VARSpecifier specifier;
    specifier = node->specifier;
    if (node->type == StructSpecifier)
    {
        specifier = STRUCTVAR;
        TreeNode *structnode = node->child;
        TreeNode *opttag = structnode->sibling;
        assert(opttag->type == OptTag);
        opttag = opttag->child;
        assert(opttag->type == ID);
        strcpy(structname, opttag->idname);
    }
    return specifier;
}
void printError(ErrorType type, int line, char *link)
{
    printf("\033[22;35mError type %d at Line %d: ", type, line);
    switch (type)
    {
    case USING_VARIBLE_BUT_NOT_DEFINED:
        printf("Undefined varible \"%s\".", link);
        break;
    case USING_FUNC_BUT_NOT_DEFINED:
        printf("Undefined function \"%s\".", link);
        break;
    case VARBILE_REDECLARATION:
        printf("Redefined varible \"%s\".", link);
        break;
    case FUNC_REDECLARATION:
        printf("Redefined function \"%s\".", link);
        break;
    case MISMATCHED_FOR_ASSIGNMENT:
        printf("Type mismatched for assignment");
        break;
    case NON_EXISTENT_FIELD:
        printf("Non-existent field \"%s\".", link);
    default:
        break;
    }
    printf("\033[0m\n");
}
void printError1(ErrorType type, int line)
{
    printf("\033[22;35mError type %d at Line %d: ", type, line);
    printf("%s", error_type_content[type]);
    printf("\033[0m\n");
}
void printError2(ErrorType type, int line, char *comment, char *link)
{
    printf("\033[22;35mError type %d at Line %d: ", type, line);
    printf("%s \"%s\". %s.", error_type_content[type], link, comment);
    printf("\033[0m\n");
}
void printError3(ErrorType type, int line, char *link)
{
    printf("\033[22;35mError type %d at Line %d: ", type, line);
    printf("%s \"%s\".", error_type_content[type], link);
    printf("\033[0m\n");
}
void printFunctionParam(char *func_name, Varkind *kind, Vartype *type)
{
    assert(kind != NULL);
    FieldList *f = kind->function.param;
    //assert(f != NULL);
    switch (type->specifier)
    {
    case INTVAR:
        printf("int ");
        break;
    case FLOATVAR:
        printf("float ");
        break;
    case STRUCTVAR:
        printf("struct ");
        break;
    default:
        break;
    }
    printf("%s(", func_name);
    for (; f != NULL; f = f->tail)
    {
        switch (f->type.specifier)
        {
        case INTVAR:
            printf("int ");
            break;
        case FLOATVAR:
            printf("float ");
            break;
        case STRUCTVAR:
            printf("struct ");
            break;
        default:
            break;
        }
        printf("%s", f->name);
        if (f->tail != NULL)
        {
            printf(",");
        }
    }
    printf(")");
}
void printExp(TreeNode *exp)
{
    //TODO:还可以打印更多的元素 以打印一个完整的 Exp
    assert(exp != NULL);
    assert(exp->type == Exp);
    TreeNode *s = exp->child;
    switch (s->type)
    {
    case INT:
        printf("%d", s->int_num);
        break;
    case FLOAT:
        printf("%g", s->float_num);
        break;
    case ID:
        //LOG("%s","来到打印函数")
        printf("%s", s->idname);
        if (s->sibling != NULL)
        {
            //ID LP Args RP 或者 ID LP RP
            s = s->sibling->sibling; //此时 s = RP 或者 Args
            printf("(");
            if (s->type == Args)
            {
                printArgs(s);
            }
            printf(")");
        }
        break;
    case LP: //LP Exp RP
        printf("(");
        printExp(s->sibling);
        printf(")");
        break;
    case NOT:
        printf("!");
        printExp(s->sibling);
        break;
    case Exp:
        printExp(s);
        s = s->sibling;
        switch (s->type)
        {
        case ASSIGNOP:
            printf("=");
            //printExp(s->sibling);
            break;
        case AND:
            printf("&&");
            break;
        case OR:
            printf("||");
            break;
        case RELOP:
            printf("神奇的符号");
            break;
        case PLUS:
            printf("+");
            break;
        case MINUS:
            printf("-");
            break;
        case STAR:
            printf("*");
            break;
        case DIV:
            printf("/");
            break;
        case LB:
            printf("[");
            break;
        case DOT:
            printf(".");
            break;
        default:
            LOG("%s", "never come here,if it occurs,just feed it to me");
            break;
        }
        if (s->type == DOT)
        {
            printf("%s", s->sibling->idname);
        }
        else
        {
            printExp(s->sibling);
            if (s->sibling->sibling != NULL)
            {
                printf("]");
            }
        }
    default:
        break;
    }
}
void printArgs(TreeNode *args)
{
    assert(args != NULL && args->type == Args);
    TreeNode *exp = args->child;
    //printExp(exp);
    if (exp->sibling == NULL)
    {
        printExp(exp);
    }
    else
    {
        printExp(exp);
        printf(",");
        printArgs(exp->sibling->sibling);
    }
}
int analyzeCheck()
{
    analyzeTree(semantic_tree_root);
    //LOG("%d", ASSIGNOP);
    //printf("analyzeTree over haha!\n");
    //printf("%d", UISNG_FUNC_BUT_NOT_DEFINED);
    return 1;
}