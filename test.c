# include <unistd.h>
# include <stdio.h>

int main (){
    int i;
    for (i =0 ;i<10; i++){
        printf("Just Keep Running i is :%d \n",i+1);
        sleep(5);
    }
    exit();
}