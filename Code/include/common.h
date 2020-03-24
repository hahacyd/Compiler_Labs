#ifndef __COMMON_H__
#define __COMMON_H__
//这个头文件是后来加的，以实现词法分析，语法分析 与 后面的语义分析共用枚举类型或函数，
typedef enum VARSpecifier //一个变量，它的Specifier 是 int , float or struct;
{
    INTVAR,
    FLOATVAR,
    STRUCTVAR,
    VOIDVAR,
    ANYVAR /* *因为有时候 类型是次要的，如j = i + 1;但是j 没有定义，但我的框架是
                * 获取j的类型，然后是i + 1的类型，j 没有定义，就没法获得其类型，这时会
                * 报两种错，变量未定义 和 类型不匹配，显然后者是次要的，可以忽略，加了
                * ANYVAR后，类型判断函数碰到ANYVAR就返回类型相等，就简接忽略了后者的错误
                * */
} VARSpecifier; //
typedef enum COMPARISON {
    EQUAL = 0, // ==
    NE, // !=
    GZ, // >
    SZ, // <
    GEZ, // >=
    SEZ // <=
} COMPARISON;

#endif