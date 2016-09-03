
#include "anaLibrary.h"
 
using namespace std; 
 
int main(int argc, char *argv[]){ 
	
	if( argc != 2 && argc != 4 && argc != 6){
		printf("usage: ./analaysis.o yindex (a Ta b Tb)\n");
		printf("         yIndex = -1 ; loop from 120 to 150 \n");
		printf("      a Ta b Tb = are optional initial guess \n");
		return 1;
	}

	int yIndex = atoi(argv[1]);
	
	///defual initial guess value, since the algorithm converge quickly, 1 guess is usually OK.
	double  a =  20;
	double Ta =  20;
	double  b = -20;
	double Tb =  60;
	
	if( argc >= 4){
		a  = atof(argv[2]);
		Ta = atof(argv[3]);
	}
	if( argc >= 6){
		b  = atof(argv[4]);
		Tb = atof(argv[5]);
	}

	///======================= get data
	char filename[100] = "20160725pentacene_pterphenyl.csv";
	printf("------------------------------\n");
	printf(" %s \n", filename);
	getData(filename);

	///====================== fitting 
	char savefile[100] = "FitResult.txt";
	
	Matrix * output;
	
	printf ("init guess (a, Ta, b, Tb) = (%3.0f, %3.0f, %3.0f, %3.0f) \n", a, Ta, b, Tb);
	if( yIndex == -1){
		system("rm FitResult.txt");
		for( int i = 0; i < sizeY; i++){
			output = Fitting(i, 0, a, Ta, b, Tb);
			char mode[5] = "a+";
			SaveFitResult(savefile, mode, i, output);
		}
	}else{
		output = Fitting(yIndex,3, a, Ta, b, Tb);
		char mode[5] = "w+";
		SaveFitResult(savefile, mode,yIndex, output);
	}
	
	///====================== gnuplot
	char plot_cmd[100];
	if( yIndex == -1 ){
		
		sprintf(plot_cmd, "gnuplot -e \"plot '%s' u 1:2 w lp \" -p", savefile );
		system(plot_cmd);
	}else{
		
		Matrix sol = output[0];
		
		double a, Ta, b, Tb;
		a  = sol(1,1);
		Ta = sol(2,1);
		b  = 0;
		Tb = 100;
		if( sol.GetRows() == 4){
			b  = sol(3,1);
			Tb = sol(4,1);
		}
		sprintf(plot_cmd, "gnuplot -e \"Col=%d;a=%f;Ta=%f;b=%f;Tb=%f;startX=%d;endX=%d\" plot.gp -p",
                    yIndex, a, Ta, b, Tb, 195, 1000);
		system(plot_cmd);
	}
	
	
	return 0; 
} 

