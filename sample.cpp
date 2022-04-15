#include <stdio.h>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(){
	printf("hello\n");


	// test open , write, read and close
	int fd = open("./sample.txt", O_RDWR | O_CREAT, 0644);
	if(fd==-1){
		perror("open");
	}

	ssize_t len;
	char buffer[1024];
	char str1[] = "1xxx2xxx3xxx4xxx5xxx\n";

	if(write(fd, str1, strlen(str1))==-1){
		perror("write");
	}

	lseek(fd, 0, SEEK_SET);

	if((len = read(fd, buffer, sizeof(buffer))) == -1){
		perror("read");
	}
	buffer[len]='\0';
	printf("<%s>",buffer);
	fflush(stdout);

	if(close(fd)==-1){
		perror("close");
	}

	// test chmod
	if(chmod("./sample.txt", 0755)==-1){
		perror("chmod");
	}

	// test chown
	if(chown("./sample.txt", -1, -1)==-1){
		perror("chown");
	}

	// test creat
	if(creat("./sample2.txt", 0666)==-1){
		perror("creat");
	}

	// test fopen, fread, fwrite, fclose
	FILE *fp;
	if((fp=fopen("./sample2.txt", "w+")) == NULL){
		perror("fopen");
	}

	// char str[] = "1abc2abc3abc4abc5abc6abc7abc8abc9abc\n";
	char str[] = "0123456789012345678901234567890123456789\n";
	size_t chunk = fwrite(str, 4, 25, fp);
	if(chunk != sizeof(str)/4){
		if(ferror(fp)){
			fprintf(stderr, "fwrite: failed!\n");
		}
	}

	fseek(fp, 0, SEEK_SET);

	chunk = fread(buffer, 4, sizeof(buffer)/4, fp);
	if(chunk != sizeof(buffer)/4){
		if(ferror(fp)){
			fprintf(stderr, "fread: failed!\n");
		}
	}
	printf("<%s>\n", buffer);
	fflush(stdout);

	if(fclose(fp)==EOF){
		perror("fclose");
	}

	// test rename ,remove
	if(rename("./sample2.txt", "./sample3.txt")==-1){
		perror("rename");
	}

	if(remove("./sample.txt")==-1)
		perror("remove sample.txt");
	if(remove("./sample3.txt")==-1)
		perror("remove sample3.txt");

	FILE *tfp = tmpfile();
	if(tfp==NULL){
		perror("tmpfile");
	}



	return 0;


}
