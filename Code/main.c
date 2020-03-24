#include"syntax.tab.c"
#include"semanticAnalyze.h"
#include"symbolTableManage.h"
#include"generateIR.h"
#include"maintainIR.h"
#include"debug.h"
int init();
char *filename = NULL;

int main(int argc, char **argv)
{
    init();
    FILE *f = NULL;
    switch (argc)
    {
    case 1:
        // LOG("usage %s test.cmm", argv[0]);
        picnic("test.cmm isn't found");
        //yyrestart(stdin);
        break;
    case 3:
        filename = argv[2];
    case 2:
        f = fopen(argv[1], "r");
        if (f != NULL)
        {
            yyrestart(f);
        }
        else{
            picnic("载入文件失败");
        }
        break;
    default:
        printf("usage:%s src [IRoutput]", argv[1]);
    }
    //yydebug = 1;
    yyparse();
    // if(!errordectected)
    //     printTree(semantic_tree_root,0);

    analyzeCheck();

    startGenerateIr(semantic_tree_root); //开始翻译语法树为中间代码
    //testrun();
    optimizeIR();
    if (filename != NULL) {
        printf("写入 %s\n", filename);
        printIr2File(filename);
    }
    //fprintf(stdout, "cyd!!!\n");
    // printSymTab();
    // printStructTab();

    return 0;
}
int init(){
    initSymbolNode();
    //加入write 和 read 函数
    Vartype *type = malloc(sizeof(Vartype));
    type->specifier = INTVAR;
    
    Varkind *kind = malloc(sizeof(Varkind));
    kind->objkind = FUNCTION;
    addFunction("read", type, kind);

    //write函数有一个 整形参数;
    kind->function.param_num = 1;
    FieldList* param = malloc(sizeof(FieldList));
    param->type.specifier = INTVAR;
    param->type.memory_size = 4;
    param->kind.objkind = VARIBLE;

    kind->function.param = param;

    addFunction("write", type, kind); 
    
}