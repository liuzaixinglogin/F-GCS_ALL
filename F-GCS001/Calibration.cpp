#include "calibration.h"
#include "QString"
#include "qmath.h"

#include "QDebug"

#define MATRIX_SIZE 7

double m_matrix[MATRIX_SIZE][MATRIX_SIZE+1];
int m = MATRIX_SIZE;	
int n = MATRIX_SIZE+1;
double m_result[MATRIX_SIZE];	

Calibration::Calibration()
{
     ResetMatrix();
}

Calibration::~Calibration()
{

}

double Calibration::Abs(double a)
{
	return a<0 ? -a : a;
}

quint8 Calibration::Equal(double a,double b)
{
	return Abs(a-b) < 1e-6;
}

void Calibration::ResetMatrix(void)
{
	int row , column;
	
	for(row = 0 ; row<m ; row++){
		for(column = 0 ; column<n ; column++)
			m_matrix[row][column] = 0.0f;
	}
}
	
void Calibration::CalcData_Input(double x , double y , double z)
{
	double V[MATRIX_SIZE];
	int row , column;
	
	V[0] = x*x;
    V[1] = y*y;
    V[2] = z*z;
    V[3] = x;
    V[4] = y;
    V[5] = z;
    V[6] = 1.0;
	
	//构建VxVt矩阵(Vt为V的转置)，并进行累加
	for(row = 0 ; row<MATRIX_SIZE ; row++){
		for(column = 0 ; column<MATRIX_SIZE ; column++){
			m_matrix[row][column] += V[row]*V[column];
		}
	}

    /*qDebug() << m_matrix[0][0] << m_matrix[0][1] << m_matrix[0][2] << m_matrix[0][3] << m_matrix[0][4] << m_matrix[0][5] << m_matrix[0][6] << endl
             << m_matrix[1][0] << m_matrix[1][1] << m_matrix[1][2] << m_matrix[1][3] << m_matrix[1][4] << m_matrix[1][5] << m_matrix[1][6] << endl
             << m_matrix[2][0] << m_matrix[2][1] << m_matrix[2][2] << m_matrix[2][3] << m_matrix[2][4] << m_matrix[2][5] << m_matrix[2][6] << endl
             << m_matrix[3][0] << m_matrix[3][1] << m_matrix[3][2] << m_matrix[3][3] << m_matrix[3][4] << m_matrix[3][5] << m_matrix[3][6] << endl
             << m_matrix[4][0] << m_matrix[4][1] << m_matrix[4][2] << m_matrix[4][3] << m_matrix[4][4] << m_matrix[4][5] << m_matrix[4][6] << endl
             << m_matrix[5][0] << m_matrix[5][1] << m_matrix[5][2] << m_matrix[5][3] << m_matrix[5][4] << m_matrix[5][5] << m_matrix[5][6] << endl
             << m_matrix[6][0] << m_matrix[6][1] << m_matrix[6][2] << m_matrix[6][3] << m_matrix[6][4] << m_matrix[6][5] << m_matrix[6][6];*/
}

void Calibration::SwapRow(int row1 , int row2)
{
	int column;
	double tmp;
	
	for(column = 0 ; column<n ; column++){
		tmp = m_matrix[row1][column];
		m_matrix[row1][column] = m_matrix[row2][column];
		m_matrix[row2][column] = tmp;
	}
}

void Calibration::MoveBiggestElement2Top(int s_row , int s_column)
{
	int row,column;
	
	for(row = s_row+1 ; row<m ; row++){
		if( Abs(m_matrix[s_row][s_column])<Abs(m_matrix[row][s_column])){
			SwapRow(s_row , row);
		}
	}
}

//高斯消元法，求行阶梯型矩阵
quint8 Calibration::Matrix_GaussElimination(void)
{
	int row,column,i,j;
	double tmp;
	
	for(row = 0,column=0 ; row<m-1 && column<n-1 ; row++,column++){
		//将当前列最大的一行移上来
		MoveBiggestElement2Top(row , column);
		
		//整列都为0
		if(Equal(m_matrix[row][column],0.0f)){
			row--;
			continue;
		}
		
		//高斯消元
		for(i = row+1 ; i<m ; i++){	
			if(Equal(m_matrix[i][column],0.0f))
				continue;	//为0，无需处理
			
			tmp = m_matrix[i][column]/m_matrix[row][column];
			
			for(j = column ; j<n ; j++){
				m_matrix[i][j] -= m_matrix[row][j]*tmp;
			}
		}
	}

	return 1;
}

//求行最简型矩阵
int Calibration::Matrix_RowSimplify(void)
{
    int c = n;//返回值，表示(解的任意常量数+1)；
    //
    int row,column,k,s,t;
    double tmp;
    //
    for(row=0,column=0;row<m && column<n;row++,column++)
    {
        if(Equal(m_matrix[row][column],0))//平移，找出本行第一个非零；
        {
            row--;
            continue;
        }
        //
        c--;//少一个常量；
        //
        //化a[i][j]为1；
        tmp = 1 / m_matrix[row][column];
        for(k=column;k<n;k++)//前面的"0"就不处理了；
            m_matrix[row][k] *= tmp;
        //
        //化a[s][j]为0
        for(s=0;s<row;s++)//下面的0也不用处理；
        {
            if(Equal(m_matrix[s][column],0))
                continue;//已经为0；
            //
            tmp = m_matrix[s][column] / m_matrix[row][column];
            for(t=column;t<n;t++)
                m_matrix[s][t] -= m_matrix[row][t]*tmp;
            //
        }
    }
    //
    return c;
}

void Calibration::Matrix_Solve(double* C , double* sol)
{
	int row,column,i;
	int any_sol[MATRIX_SIZE];

	//找出任意解的位置
	memset(any_sol , 0 , MATRIX_SIZE);
	for(row=0,column=0 ; row<m && column<n-1 ; row++,column++){
		if(Equal(m_matrix[row][column] , 0.0f)){
			any_sol[column] = 1;	//记录任意解的位置
			row--;	//右移1列
		}
	}

	//求解
	row = 0;
	for(column = 0 ; column<n-1 ; column++){
		if(any_sol[column] == 1){	//任意解
			sol[column] = C[column];
		}else{
			sol[column] = m_matrix[row][n-1];
			//加上任意解
			for(i = column+1 ; i<n-1 ; i++){
				if(any_sol[i]==1 && !Equal(m_matrix[row][i],0.0f)){
					sol[column] -= m_matrix[row][i]*C[i];
				}
			}	
			row++;
		}
	}
}

void Calibration::Calc_Process(double radius)
{
	double C[MATRIX_SIZE];
	double Res[MATRIX_SIZE];
	int i;
	double k;

    //ResetMatrix();

	//输入任意个数磁场测量点坐标，请尽量保证在椭球上分布均匀
    //CalcData_Input(7 , -7 , -2);
    //CalcData_Input(-1 , -7 , -2);
    //CalcData_Input(3 , 3 , -2);
    //CalcData_Input(3 , -17 , -2);
    //CalcData_Input(3 , -7 , 4);

    /*qDebug() << m_matrix[0][0] << m_matrix[0][1] << m_matrix[0][2] << m_matrix[0][3] << m_matrix[0][4] << m_matrix[0][5] << m_matrix[0][6] << endl
             << m_matrix[1][0] << m_matrix[1][1] << m_matrix[1][2] << m_matrix[1][3] << m_matrix[1][4] << m_matrix[1][5] << m_matrix[1][6] << endl
             << m_matrix[2][0] << m_matrix[2][1] << m_matrix[2][2] << m_matrix[2][3] << m_matrix[2][4] << m_matrix[2][5] << m_matrix[2][6] << endl
             << m_matrix[3][0] << m_matrix[3][1] << m_matrix[3][2] << m_matrix[3][3] << m_matrix[3][4] << m_matrix[3][5] << m_matrix[3][6] << endl
             << m_matrix[4][0] << m_matrix[4][1] << m_matrix[4][2] << m_matrix[4][3] << m_matrix[4][4] << m_matrix[4][5] << m_matrix[4][6] << endl
             << m_matrix[5][0] << m_matrix[5][1] << m_matrix[5][2] << m_matrix[5][3] << m_matrix[5][4] << m_matrix[5][5] << m_matrix[5][6] << endl
             << m_matrix[6][0] << m_matrix[6][1] << m_matrix[6][2] << m_matrix[6][3] << m_matrix[6][4] << m_matrix[6][5] << m_matrix[6][6];
*/
	Matrix_GaussElimination();
	Matrix_RowSimplify();

    //赋值任意解参数值
	for(i = 0 ; i<MATRIX_SIZE ; i++){
		C[i] = 1.0f;
	}

    Matrix_Solve(C , Res);
    qDebug() << "a:" << Res[0]
             << "b:" << Res[1]
             << "c:" << Res[2]
             << "d:" << Res[3]
             << "e:" << Res[4]
             << "f:" << Res[5]
             << "g:" << Res[6];

	k = (Res[3]*Res[3]/Res[0]+Res[4]*Res[4]/Res[1]+Res[5]*Res[5]/Res[2] - 4*Res[6])/(4*radius*radius);

	m_result[0] = sqrt(Res[0] / k);
    m_result[1] = sqrt(Res[1] / k);
    m_result[2] = sqrt(Res[2] / k);
    m_result[3] = Res[3] / (2 * Res[0]);
    m_result[4] = Res[4] / (2 * Res[1]);
    m_result[5] = Res[5] / (2 * Res[2]);

    qDebug() << "Xo:" << m_result[3]
             << "Yo:" << m_result[4]
             << "Zo:" << m_result[5]
             << "Xg:" << m_result[0]
             << "Yg:" << m_result[1]
             << "Zg:" << m_result[2]
             << "k:"  << k;
}


