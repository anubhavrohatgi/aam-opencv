/*
    COP - Active Appearance Model Face Tracking - using Inverse Compositional Approach
    Rohan Anil , under Dr. Radhika Vathsan , BITS Pilani Goa Campus
    rohan.ani;@gmail.com
    License : GPLV3
*/
#include "cv.h"
#include "highgui.h"
#include "LKInverseCompRobustClass.h"
#include <math.h>

#include <stdio.h>

LKInverseComp::LKInverseComp()
{
// Create matrices.
    WarpMatrix = cvCreateMat(3, 3, CV_32F);
    cvSet(WarpMatrix, cvScalar(0));

    WarpMatrixDeltaP = cvCreateMat(3, 3, CV_32F);
    cvSet(WarpMatrixDeltaP, cvScalar(0));


    WarpMatrixDeltaPInverse = cvCreateMat(3, 3, CV_32F);
    cvSet(WarpMatrixDeltaPInverse, cvScalar(0));


    HessianMatrix  = cvCreateMat(6, 6, CV_32F);
    cvSet(HessianMatrix, cvScalar(0));

    HessianMatrixInverse= cvCreateMat(6, 6, CV_32F);
    cvSet(HessianMatrixInverse, cvScalar(0));

    DeltaP = cvCreateMat(6, 1, CV_32F);
    cvSet(HessianMatrixInverse, cvScalar(0));

    for (int i=0;i<16;i++)
    {
        MVALUE[i]=1;
        HessianBlockMatrix[i]=cvCreateMat(6, 6, CV_32F);
        steepestDescentImageTranspose_X_errorImage_MVALUE_perBlock[i]=cvCreateMat(6, 1, CV_32F);
        cvSet(steepestDescentImageTranspose_X_errorImage_MVALUE_perBlock[i], cvScalar(0));
        cvSet(HessianBlockMatrix[i], cvScalar(0));
    }


}
void LKInverseComp::iterate()
{

    for (int i=0;i<16;i++)
    {
        MVALUE[i]=1;

    }
    int k=numberOfIterations;
//printf("%d %d\n",templateImage->width,templateImage->height);
    int pixelCount=0;
    double totalError=0;
    double averageError=0;

    while (k>0)
    {
     //    printf("%f %f \n ", CV_MAT_ELEM(*WarpMatrix, float,0, 0),CV_MAT_ELEM(*WarpMatrix, float,0, 1));
   //printf("%f %f \n ", CV_MAT_ELEM(*WarpMatrix, float,1, 0),CV_MAT_ELEM(*WarpMatrix, float,1,1 ));

   //printf("%f %f \n", CV_MAT_ELEM(*WarpMatrix, float,0, 2),CV_MAT_ELEM(*WarpMatrix, float,1, 2));

        k--;

        for (int l=0;l<4;l++)
        {
            for (int m=0;m<4;m++)
            {

                for (int j=l*(templateImage->height/4); j<(l*(templateImage->height/4)+(templateImage->height/4)); j++)
                {
                    for (int i=m*(templateImage->width/4); i<(m*(templateImage->width/4) + (templateImage->width/4)); i++)
                    {

                        int newI =i-m*(templateImage->width/4);
                        int newJ =j-l*(templateImage->height/4);


                        int index=l*4 +m;

                        CV_MAT_ELEM(*errorImage[index], float, newI*(templateImage->height/4) + newJ, 0) = 0;

                        float templateValue ;
                        CvScalar s;
                        s=cvGet2D(templateImage,j,i);
                        templateValue=s.val[0];

                        float Wi = CV_MAT_ELEM(*WarpMatrix, float,0, 0)*i + CV_MAT_ELEM(*WarpMatrix, float,0, 1)*j +  CV_MAT_ELEM(*WarpMatrix, float,0, 2);
                        float Wj = CV_MAT_ELEM(*WarpMatrix, float,1, 0)*i + CV_MAT_ELEM(*WarpMatrix, float,1, 1)*j +  CV_MAT_ELEM(*WarpMatrix, float,1, 2);

                        int Wi_floor = cvFloor(Wi);
                        int Wj_floor = cvFloor(Wj);


                        if (Wi_floor>=0 && Wi_floor<iterateImage->width && Wj_floor>=0 && Wj_floor<iterateImage->height)
                        {
                            pixelCount++;
                            float imageValue =interpolate<uchar>(iterateImage, Wi, Wj);
                            totalError+=imageValue-templateValue;
                            CV_MAT_ELEM(*errorImage[index], float, newI*(templateImage->height/4) + newJ, 0) = (imageValue-templateValue);
                        }
                    }



                }


            }
        }



        for (int l=0;l<4;l++)
        {
            for (int m=0;m<4;m++)
            {
                int index=l*4 +m;

                float errorSquare=0;
                float squareOfSumOfTemplatePixel=0;
                float squareOfIndividualTemplatePixel=0;
                for (int j=l*(templateImage->height/4); j<(l*(templateImage->height/4)+(templateImage->height/4)); j++)
                {
                    for (int i=m*(templateImage->width/4); i<(m*(templateImage->width/4) + (templateImage->width/4)); i++)
                    {

                        int newI =i-m*(templateImage->width/4);
                        int newJ =j-l*(templateImage->height/4);
                        CvScalar s;
                        s=cvGet2D(templateImage,j,i);

                        squareOfSumOfTemplatePixel+=s.val[0];
                        squareOfIndividualTemplatePixel+=pow(s.val[0],2);


                        errorSquare+=pow(CV_MAT_ELEM(*errorImage[index], float, (newI*templateImage->height/4) + newJ, 0),2);



                    }
                }

                squareOfSumOfTemplatePixel=pow(squareOfSumOfTemplatePixel,2);
                float variance=squareOfIndividualTemplatePixel - (squareOfSumOfTemplatePixel/((templateImage->height*templateImage->width)/16));

                if (variance!=0)
                    errorSquare/=variance;


                errorValues[index]=errorSquare;
//printf("%e \n",errorValues[index]);

            }
        }
        for (int i=0;i<5;i++)
        {
            int max=errorValues[0];
            int maxindex=0;
            for (int j=0;j<16;j++)
            {
                if (max<errorValues[j])
                {
                    max=errorValues[j];
                    maxindex=j;
                }
            }
            if(maxindex!=-1)
            {
            MVALUE[maxindex]=0;
            errorValues[maxindex]=-100000000;
            }
//printf("%d MAX Index \n",maxindex);

        }

        if (pixelCount!=0)
            averageError =totalError/(double)pixelCount;


        cvSet(HessianMatrix, cvScalar(0));
        cvSet(steepestDescentImageTranspose_X_errorImage_MVALUE, cvScalar(0));



        for (int i=0;i<16;i++)
        {
            cvMatMul(steepestDescentImageTransposeMatrix[i],errorImage[i], steepestDescentImageTranspose_X_errorImage_MVALUE_perBlock[i]);
            cvMatMul(steepestDescentImageTransposeMatrix[i],steepestDescentImageBlockMatrix[i],HessianBlockMatrix[i]);
            cvConvertScale(HessianBlockMatrix[i],HessianBlockMatrix[i],MVALUE[i], 0);
            cvAdd(HessianBlockMatrix[i],HessianMatrix,HessianMatrix);
            cvConvertScale(steepestDescentImageTranspose_X_errorImage_MVALUE_perBlock[i],steepestDescentImageTranspose_X_errorImage_MVALUE_perBlock[i],MVALUE[i], 0);
            cvAdd(steepestDescentImageTranspose_X_errorImage_MVALUE_perBlock[i],steepestDescentImageTranspose_X_errorImage_MVALUE,steepestDescentImageTranspose_X_errorImage_MVALUE);

        }
        cvInvert(HessianMatrix,HessianMatrixInverse);



        //cvMatMul(HessianMatrixInverse_steepestDescentImageTranspose,errorImage,DeltaP);
        cvMatMul(HessianMatrixInverse,steepestDescentImageTranspose_X_errorImage_MVALUE,DeltaP);

        float p1= CV_MAT_ELEM(*DeltaP, float,0, 0);
        float p2= CV_MAT_ELEM(*DeltaP,float, 1, 0);
        float p3= CV_MAT_ELEM(*DeltaP,float, 2, 0);
        float p4= CV_MAT_ELEM(*DeltaP,float, 3, 0);
        float p5= CV_MAT_ELEM(*DeltaP,float, 4, 0);
        float p6= CV_MAT_ELEM(*DeltaP,float, 5, 0);
        //     printf ("%f %f %f %f %f %f %f \n",p1,p2,p3,p4,p5,p6);
        //  printf("%e MEAN ERROR \n",averageError);
        setWarpMatrix(WarpMatrixDeltaP,p1,p2,p3,p4,p5,p6);
                cvSet(WarpMatrixDeltaPInverse, cvScalar(0));

        cvInvert(WarpMatrixDeltaP,WarpMatrixDeltaPInverse);

        cvMatMul(WarpMatrix,WarpMatrixDeltaPInverse,WarpMatrix);


    if (fabs(p1)<=DeltaE && fabs(p2)<=DeltaE && fabs(p3)<=DeltaE && fabs(p4)<=DeltaE && fabs(p5)<=DeltaE && fabs(p6)<=DeltaE)
     {
printf("Broken on Kth Iteration %d \n",k);
            break;
        }
    }

}
void LKInverseComp:: setTemplate(IplImage* image)
{
    templateImage=image;
    preCompute();
}

void LKInverseComp:: setImage(IplImage* image)
{
    iterateImage=image;
}

void LKInverseComp:: setCurrentWarpEstimate(float p1,float p2,float p3 ,float p4,float p5,float p6)
{
    setWarpMatrix(WarpMatrix,p1,p2,p3,p4,p5,p6);
}

void LKInverseComp::setWarpMatrix(CvMat* Warp,float p1=0,float p2=0,float p3 =0,float p4=0 ,float p5=0,float p6=0)
{
    CV_MAT_ELEM(*Warp, float,0, 0)=1+p1;
    CV_MAT_ELEM(*Warp, float,1, 0)=p2;

    CV_MAT_ELEM(*Warp, float,0, 1)=p3;
    CV_MAT_ELEM(*Warp, float,1, 1)=1+p4;

    CV_MAT_ELEM(*Warp, float,0, 2)=p5;
    CV_MAT_ELEM(*Warp, float,1, 2)=p6;

    CV_MAT_ELEM(*Warp, float,2, 0)=0;
    CV_MAT_ELEM(*Warp, float,2, 1)=0;
    CV_MAT_ELEM(*Warp, float,2, 2)=1;
}
void LKInverseComp::preCompute()
{

    GradientX = cvCreateImage(cvSize(templateImage->width,templateImage->height),  IPL_DEPTH_32F, 1);
    GradientY = cvCreateImage(cvSize(templateImage->width,templateImage->height),  IPL_DEPTH_32F, 1);
cvZero(GradientX);
cvZero(GradientY);
cvSobel(templateImage, GradientX, 1, 0);
cvSobel(templateImage, GradientY, 0, 1);

 cvConvertScale(GradientX, GradientX, .1);
 cvConvertScale(GradientY, GradientY, .1);

    steepestDescentImage  = cvCreateMat((templateImage->width*templateImage->height), 6, CV_32F);
    cvSet(steepestDescentImage, cvScalar(0));
    for (int i=0;i<16;i++)
    {

        steepestDescentImageBlockMatrix[i]=cvCreateMat((templateImage->width*templateImage->height)/16,6, CV_32F);
        cvSet(steepestDescentImageBlockMatrix[i], cvScalar(0));
        steepestDescentImageTransposeMatrix[i]=cvCreateMat(6, (templateImage->width*templateImage->height)/16, CV_32F);
        cvSet(steepestDescentImageTransposeMatrix[i], cvScalar(0));
        errorImage[i] = cvCreateMat((templateImage->width*templateImage->height/16),1, CV_32F);
        cvSet(errorImage[i], cvScalar(0));
        cvSet(steepestDescentImageTranspose_X_errorImage_MVALUE_perBlock[i], cvScalar(0));


    }




    steepestDescentImageTranspose  = cvCreateMat( 6,(templateImage->width*templateImage->height), CV_32F);
    cvSet(steepestDescentImageTranspose, cvScalar(0));


    steepestDescentImageTranspose_X_errorImage_MVALUE =cvCreateMat( 6,1, CV_32F);
    cvSet(steepestDescentImageTranspose_X_errorImage_MVALUE, cvScalar(0));


    /*
    Find Gradient of The Image
    */

    for (int l=0;l<4;l++)
    {
        for (int m=0;m<4;m++)
        {

            for (int j=l*(templateImage->height/4); j<(l*(templateImage->height/4)+(templateImage->height/4)); j++)
            {
                for (int i=m*(templateImage->width/4); i<(m*(templateImage->width/4) + (templateImage->width/4)); i++)
                {
                    int newI =i-m*(templateImage->width/4);
                    int newJ =j-l*(templateImage->height/4);




                    int index=l*4 +m;

                    float GradT_x;
                    float GradT_y;

                    CvScalar s;
                    s=cvGet2D(GradientX,j,i);
                    GradT_x=s.val[0];
                    s=cvGet2D(GradientY,j,i);
                    GradT_y=s.val[0];
//
                    //  printf("Grad tx = %e \n",GradT_x);
                    CV_MAT_ELEM(*steepestDescentImageBlockMatrix[index], float, (newI*templateImage->height/4) + newJ, 0) = GradT_x*i;
                    CV_MAT_ELEM(*steepestDescentImageBlockMatrix[index], float, (newI*templateImage->height/4) + newJ, 1) = GradT_y*i;
                    CV_MAT_ELEM(*steepestDescentImageBlockMatrix[index], float, (newI*templateImage->height/4) + newJ, 2) = GradT_x*j;
                    CV_MAT_ELEM(*steepestDescentImageBlockMatrix[index], float, (newI*templateImage->height/4) + newJ, 3) = GradT_y*j;
                    CV_MAT_ELEM(*steepestDescentImageBlockMatrix[index], float, (newI*templateImage->height/4) + newJ, 4) = GradT_x;
                    CV_MAT_ELEM(*steepestDescentImageBlockMatrix[index], float, (newI*templateImage->height/4) + newJ, 5) = GradT_y;

                }

            }
        }
    }

    for (int l=0;l<4;l++)
    {
        for (int m=0;m<4;m++)
        {
            int index=l*4 +m;
            cvTranspose(steepestDescentImageBlockMatrix[index], steepestDescentImageTransposeMatrix[index]);
            cvMatMul(steepestDescentImageTransposeMatrix[index],steepestDescentImageBlockMatrix[index],HessianBlockMatrix[index]);
        }
    }
    cvSet(HessianMatrix, cvScalar(0));

    //  cvMatMul(steepestDescentImageTranspose,steepestDescentImage,HessianMatrix);


    //  double inv = cvInvert(HessianMatrix,HessianMatrixInverse);
    //  cvMatMul(HessianMatrixInverse,steepestDescentImageTranspose,HessianMatrixInverse_steepestDescentImageTranspose);

}
