#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <stdio.h>
#include <string>
#include <time.h>

using namespace std;


int ReturnStrings(const char* path, const int num, char* buf, const int size)
{
    int cnt = 0;
    FILE* file = NULL;

    if ((file = fopen(path, "r")) == NULL)
        return -1;

    while (fgets(buf, size, file) != NULL && num > ++cnt);
    fclose(file);

    if (num != cnt)
    {
        for (cnt = 0; cnt < size; ++cnt)
            buf[cnt] = 0;
        return -2;
    }

    return 0;
}

int makeFile(int t){
	
	char AODV_0[128];
	char OLSR_0[128];
	char GPSR_0[128];
	char IGPSR_0[128];
	char PGPSR_0[128];
	char LGPSR_0[128];
	
	double AODV[t],OLSR[t],GPSR[t],IGPSR[t],PGPSR[t],LGPSR[t];
	std::string value = "NULL";	
	
	for(int i=1;i<=6;i++){
		for(int l=1;l<=t;l++){
			
			std::string Astr = "/home/hry-user/Station/AODV/data" +std::to_string(l)+ ".txt";
			std::string Ostr = "/home/hry-user/Station/OLSR/data" +std::to_string(l)+ ".txt";
			std::string Gstr = "/home/hry-user/Station/GPSR/data" +std::to_string(l)+ ".txt";
			std::string Istr = "/home/hry-user/Station/IGPSR/data" +std::to_string(l)+ ".txt";
			std::string Pstr = "/home/hry-user/Station/PGPSR/data" +std::to_string(l)+ ".txt";
			std::string Lstr = "/home/hry-user/Station/LGPSR/data" +std::to_string(l)+ ".txt";
			
			ReturnStrings( Astr.c_str(), i, AODV_0, 128 );
			ReturnStrings( Ostr.c_str(), i, OLSR_0, 128 );
			ReturnStrings( Gstr.c_str(), i, GPSR_0, 128 );
			ReturnStrings( Istr.c_str(), i, IGPSR_0, 128 );
			ReturnStrings( Pstr.c_str(), i, PGPSR_0, 128 );
			ReturnStrings( Lstr.c_str(), i, LGPSR_0, 128 );
			
			AODV[l] = atof(AODV_0);
			OLSR[l] = atof(OLSR_0);
			GPSR[l] = atof(GPSR_0);
			IGPSR[l] = atof(IGPSR_0);
			PGPSR[l] = atof(PGPSR_0);
			LGPSR[l] = atof(LGPSR_0);
		}
		switch(i){		
		case 1:
		value="Throughput";
		break;
		case 2:
		value="PDR";
		break;
		case 3:
		value="Overhead";
		break;
		case 4:
		value="Delay";
		break;
		case 5:
		value="Loss";
		break;
		case 6:
		value="HopNum";	
		break;
		default:
		value="NULL";			
		}
		fstream fs;
		fs.open("/home/hry-user/Station/"+value+".txt", ios::out);
		fs << value<<","<<"AODV"<<","<<"OLSR"<<","<<"GPSR"<<","<<"IGPSR"<<","<<"PGPSR"<<","<<"LGPSR"<<endl;
		
		for(int r=1; r<=t; r++){
			fs <<std::to_string(r)<<"回目"<<","<<std::to_string(AODV[r])<<","<<std::to_string(OLSR[r])<<","<<std::to_string(GPSR[r])<<","<<std::to_string(IGPSR[r])<<","<<std::to_string(PGPSR[r])<<","<<std::to_string(LGPSR[r])<<endl;
			//printf("%f\n",LGPSR[l]);
		}
		fs.close();		
			
		}
		return 0;
	
	}
int main(int argc, char *argv[]){

	int t = atoi(argv[1]);//実験回数
	makeFile(t);
	
	
	return 0;

}
