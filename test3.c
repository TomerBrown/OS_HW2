#include <stdio.h>
int main(){
    char str [1024];
    scanf("%s",str);
    char str2[6];
    for (int i=0;i<5;i++){
        str2[i] = str[i];
    }
    str2[5] = 0;
    printf("%s\n", str2);
    return 0;
}