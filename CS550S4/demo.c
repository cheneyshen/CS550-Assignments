#include<stdio.h>
#include<stdlib.h> //引用指针空间

struct student //定义一个结构体student
{
	int sAge;//属性，4字节
	int sName;
};
int main()
{
	int n=20;//定义结构体的大小的变量，可以使任意数值
	struct student *pA; //只能用指针的形式动态申请内存
	pA=(struct student *)malloc(sizeof(struct student)*n);//每一行的大小（4字节）*行数
	pA[0].sAge=20;//这里是箭头，不是小数点
	printf("%d\n",pA->sAge);
	//pA++;//一次递加4
	pA[1].sAge=21;
	printf("%d, %d\n",pA[0].sAge, pA[1].sAge);
}
