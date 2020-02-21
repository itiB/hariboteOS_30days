int _strcmp(char *s1, char *s2){
    while(*s1 != '\0' && *s2 != '\0'){
        if(*s1 != *s2) {
            break;
        }
        s1++;
        s2++;
    }

    return(*s1 - *s2);
}

int _strlen(char *str){
    int i;
    for(i = 0; str[i] != '\0'; i++);
    return(i);
}
