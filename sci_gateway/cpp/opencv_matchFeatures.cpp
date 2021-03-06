/********************************************************************************************************************************************************************************************************
*	Author:	Umang Agrawal & Siddhant Narang																		*
* 	Code:	matchFeatures.cpp																					*
*	Function Format:	[ index_Pairs_of_Matched_fetaures Metric ] = matchFeatures( feature_Vector_1, feature_Vector_2, Optional Arguments )							*
*	Optional Arguments:		Name			 Value															*
*							'Method'		 [ 'Exhaustive' : 'Brute Force Matching', 'Approximate' : FANN Based Matching' ]						*
*							'Metric'		 [ 'SSD', 'SAD', 'Hamming', 'Hamming_2' ]										*
*							'Unique'		 [ Boolean True or False ]												*
*							'MatchThreshold' [ Percentage: 0 - 100 ]													*
********************************************************************************************************************************************************************************************************/
#include <iostream>
#include <numeric>
#include <vector>
#include <string>
#include "opencv2/core/core.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

using namespace cv;
using namespace std;

extern "C"
{
	#include "api_scilab.h"
	#include "Scierror.h"
	#include "BOOL.h"
	#include <localization.h>
	#include "sciprint.h"
	#include "../common.h"

	int opencv_matchFeatures(char *fname, unsigned long fname_len)
	{
		// Error Handling		
		SciErr sciErr;

		// Address of Various Scilab API
		int *piAddr1 = NULL;
		int *piAddr2 = NULL;
		int *piAddr3 = NULL;
		int *piAddr4 = NULL;

		// Number of input and output Arguments
		int inputArguments = 0;

		int Rows_1,Cols_1;
		int Rows_2,Cols_2;

		// Input arguments
		double *feature_1 = NULL;
		double *feature_2 = NULL;

		int iRows, iCols;	
		int *pilen = NULL;
		char **arg = NULL;
		char **method = NULL;
		char **metric = NULL;
		double unique = 0;
		double thresh = 10;

		int count_method = 0,count_metric = 0, count_unique = 0, count_thresh = 0;		

		double max_dist = 0, min_dist;
		double *indexPairs = NULL;
		double *matchMetric = NULL;

		vector < vector<DMatch> > matches1;
		vector <DMatch> matches;
		vector <DMatch> valid_m;		

		// Checks on Number of Input and Output Arguments
		CheckInputArgument(pvApiCtx, 2, 10);
		CheckOutputArgument(pvApiCtx, 1, 2);

		inputArguments = *getNbInputArgument(pvApiCtx);

		if( inputArguments%2 == 0 )
		{
			// Feature Matrix 1			
			sciErr = getVarAddressFromPosition(pvApiCtx, 1, &piAddr1);
			if(sciErr.iErr)
			{
				printError(&sciErr, 0);
				return 0;
			}
			if( !isVarMatrixType(pvApiCtx, piAddr1))
			{
				Scierror(999,"Expecting a Matrix of MxN order containing the Binary Feature Set of Image 1 (Query Image)\n");
				return 0;
			}
			if( !isDoubleType(pvApiCtx, piAddr1))
			{
				Scierror(999,"Feature Matrix should be of Double Type\n");
				return 0;
			}
			sciErr = getMatrixOfDouble(pvApiCtx, piAddr1, &Rows_1, &Cols_1, &feature_1);
			if(sciErr.iErr)			
			{
				printError(&sciErr, 0);
				return 0;
			}
 
			// Feature Matrix 2
			sciErr = getVarAddressFromPosition(pvApiCtx, 2, &piAddr2);
			if(sciErr.iErr)
			{
				printError(&sciErr, 0);
				return 0;
			}
			if( !isVarMatrixType(pvApiCtx, piAddr2))
			{
				Scierror(999,"Expecting a Matrix of MxN order containing the Binary Feature Set of Image 1 (Query Image)\n");
				return 0;
			}
			if( !isDoubleType(pvApiCtx, piAddr2))
			{
				Scierror(999,"Feature Matrix should be of Double Type\n");
				return 0;
			}
			sciErr = getMatrixOfDouble(pvApiCtx, piAddr2, &Rows_2, &Cols_2, &feature_2);
			if(sciErr.iErr)			
			{
				printError(&sciErr, 0);
				return 0;
			}
		}
		else
		{
			Scierror(999,"Required Arguments are Missing\n");
			return 0;
		}

		for( int i=3; i<=inputArguments; i++)
		{
			sciErr = getVarAddressFromPosition(pvApiCtx, i, &piAddr3);
			if(sciErr.iErr)
			{
				printError(&sciErr, 0);
				return 0;
			}
			// Check for Argument type
			if( !isStringType(pvApiCtx, piAddr3))
			{
				Scierror(999, "%s: Wrong type of argument #%d. A string is expected.\n", fname, 1);
				return 0;
			}
			// Matrix of Stings
			sciErr = getMatrixOfString(pvApiCtx, piAddr3, &iRows, &iCols, NULL, NULL);
			if(sciErr.iErr)
			{
				printError(&sciErr, 0);
				return 0;
			}
			pilen = (int*)malloc(sizeof(int) * iRows * iCols);
			
			// second call to retrieve the length of the string
			sciErr = getMatrixOfString(pvApiCtx, piAddr3, &iRows, &iCols, pilen, NULL);
			if(sciErr.iErr)
			{
				printError(&sciErr, 0);
				free(pilen);
				return 0;
			}
			arg = (char**)malloc(sizeof(char*) * iRows * iCols);
			for(int j=0;j< iRows * iCols; j++)
			{
				arg[j] = (char*)malloc(sizeof(char) * (pilen[j] + 1));
			}
			
			// third call to retrieve data
			sciErr = getMatrixOfString(pvApiCtx, piAddr3, &iRows, &iCols, pilen, arg);
			if(sciErr.iErr)
			{
				printError(&sciErr, 0);
				free(pilen);
				free(arg);
				return 0;
			}

			if(strcmp(arg[0],"Method") == 0)
			{
				if(count_method != 0)
				{
					Scierror(999,"Method argument has been called twice.\n");
					return 0;
				}
				free(arg);
				free(pilen);
				sciErr = getVarAddressFromPosition(pvApiCtx, i+1, &piAddr4);
				if(sciErr.iErr)
				{
					printError(&sciErr, 0);
					return 0;
				}
				
				// Check for Argument type
				if( !isStringType(pvApiCtx, piAddr4))
				{
					Scierror(999, "%s: Wrong type of value for Method Argument. A string is expected.\n");
					return 0;
				}
				
				// Matrix of Stings
				sciErr = getMatrixOfString(pvApiCtx, piAddr4, &iRows, &iCols, NULL, NULL);	
				if(sciErr.iErr)
				{
					printError(&sciErr, 0);
					return 0;
				}
				pilen = (int*)malloc(sizeof(int) * iRows * iCols);
				
				// second call to retrieve the length of the string
				sciErr = getMatrixOfString(pvApiCtx, piAddr4, &iRows, &iCols, pilen, NULL);
				if(sciErr.iErr)
				{
					printError(&sciErr, 0);
					free(pilen);
					return 0;
				}
				method = (char**)malloc(sizeof(char*) * iRows * iCols);
				for(int j=0;j< iRows * iCols; j++)
				{
					method[j] = (char*)malloc(sizeof(char) * (pilen[j] + 1));
				}
				
				// third call to retrieve data
				sciErr = getMatrixOfString(pvApiCtx, piAddr4, &iRows, &iCols, pilen, method);
				if(sciErr.iErr)
				{
					printError(&sciErr, 0);
					free(pilen);
					free(method);
					return 0;
				}
				i++;
				count_method += 1;
			}
			
			else if(strcmp(arg[0],"Metric") == 0)
			{
				if(count_metric != 0)
				{
					Scierror(999,"Metric argument has been called twice.\n");
					return 0;
				}
				free(arg);
				free(pilen);
				
				sciErr = getVarAddressFromPosition(pvApiCtx, i+1, &piAddr4);
				if(sciErr.iErr)
				{
					printError(&sciErr, 0);
					return 0;
				}
				
				//Check for Argument type
				if( !isStringType(pvApiCtx, piAddr4))
				{
					Scierror(999, "%s: Wrong type of Value for Metric Argument. A string is expected.\n");
					return 0;
				}
				
				//Matrix of Stings
				sciErr = getMatrixOfString(pvApiCtx, piAddr4, &iRows, &iCols, NULL, NULL);	
				if(sciErr.iErr)
				{
					printError(&sciErr, 0);
					return 0;
				}
				pilen = (int*)malloc(sizeof(int) * iRows * iCols);
				
				//second call to retrieve the length of the string
				sciErr = getMatrixOfString(pvApiCtx, piAddr4, &iRows, &iCols, pilen, NULL);
				if(sciErr.iErr)
				{
					printError(&sciErr, 0);
					free(pilen);
					return 0;
				}
				metric = (char**)malloc(sizeof(char*) * iRows * iCols);
				for(int j=0;j< iRows * iCols; j++)
				{
					metric[j] = (char*)malloc(sizeof(char) * (pilen[j] + 1));
				}
				
				//third call to retrieve data
				sciErr = getMatrixOfString(pvApiCtx, piAddr4, &iRows, &iCols, pilen, metric);
				if(sciErr.iErr)
				{
					printError(&sciErr, 0);
					free(pilen);
					free(metric);
					return 0;
				}
				i++;
				count_metric += 1;	
			}
			
			else if(strcmp(arg[0],"Unique") == 0)
			{
				if(count_unique != 0)
				{
					Scierror(999,"Unique argument has been called twice.\n");
					return 0;
				}
				free(arg);
				free(pilen);
				
				sciErr = getVarAddressFromPosition(pvApiCtx, i+1, &piAddr4);
				if(sciErr.iErr)
				{
					printError(&sciErr, 0);
					return 0;
				}
				
				//Check for Argument type
				if( !(isDoubleType(pvApiCtx, piAddr4)) )
				{
					Scierror(999,"Unique Value must be a logic scalar\n");
					return 0;
				}
				if(getScalarDouble(pvApiCtx, piAddr4, &unique))
				{
					Scierror(999,"Cannot Read Upright Value\n");
					return 0;
				}
				if( !(int(unique)==0 || int(unique)==1) )
				{
					Scierror(999,"Unique Value must be a logic scalar\n");
					return 0;
				}
				i++;
				count_unique += 1;
			}
			
			else if(strcmp(arg[0],"MatchThreshold") == 0)
			{
				if(count_thresh != 0)
				{
					Scierror(999,"Match Threshold argument has been called twice.\n");
					return 0;
				}
				free(arg);
				free(pilen);
				
				sciErr = getVarAddressFromPosition(pvApiCtx, i+1, &piAddr4);
				if(sciErr.iErr)
				{
					printError(&sciErr, 0);
					return 0;
				}
				
				//Check for Argument type
				if( !(isDoubleType(pvApiCtx, piAddr4)) )
				{
					Scierror(999,"Match Threshold Value must be a scalar Double\n");
					return 0;
				}
				if(getScalarDouble(pvApiCtx, piAddr4, &thresh))
				{
					Scierror(999,"Cannot Read Threshold Value\n");
					return 0;
				}
				if( !(thresh>0 && thresh<=100) )
				{
					Scierror(999,"Threshold Value must be in the range of 1 to 100\n");
					return 0;
				}
				i++;
				count_thresh += 1;
			}
			
			else
			{
				Scierror(999, "Invalid Argument Name. Provide Valid Arguments\n");
				return 0;
			}	
		}

		if( count_metric == 0 )
		{
			metric = (char**)malloc(sizeof(char*) * 1 * 1);
			metric[0] = (char*)malloc(sizeof(char) * 4);
			strcpy(metric[0], "SSD");	
		}
		
		if( count_method == 0)
		{
			method = (char**)malloc(sizeof(char*) * 1 * 1);
			method[0] = (char*)malloc(sizeof(char) * 11);
			strcpy(method[0], "Exhaustive");	
		}

		Mat descriptor_2(Rows_2, Cols_2, CV_32FC1);
		if( strcmp(method[0],"Exhaustive") == 0)
		{
			Mat descriptor_1(Rows_1, Cols_1, CV_8UC1);
			Mat descriptor_2(Rows_2, Cols_2, CV_8UC1);
			
			// Converting feature_1 vector from double matrix to cv::mat.
			for(int i = 0; i < Rows_1; i++)
			{
				for(int j = 0; j < Cols_1; j++)
				{
					descriptor_1.at<uchar>(i,j) = int(feature_1[j*Rows_1 + i]);
				}
			}
			
			// Converting feature_2 vector from double matrix to cv::mat.
			for(int i = 0; i < Rows_2; i++)
			{
				for(int j = 0; j < Cols_2; j++)
				{
					descriptor_2.at<uchar>(i,j) = int(feature_2[j*Rows_2 + i]);
				}
			}

			if( strcmp(metric[0],"SSD") == 0)
			{	
				Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create("BruteForce");
				try
				{
					matcher->match(descriptor_1, descriptor_2, matches);	
				}
				catch(Exception &e)
				{
					const char *err = e.what();
					Scierror(999, "%s", err);
					return 0;
				}
				// BFMatcher matcher(NORM_L2, int(unique));
				// matcher.match(descriptor_1, descriptor_2, matches);
			}
			else if( strcmp(metric[0],"SAD") == 0)
			{
				Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create("BruteForce-L1");
				try
				{
					matcher->match(descriptor_1, descriptor_2, matches);	
				}
				catch(Exception &e)
				{
					const char *err = e.what();
					Scierror(999, "%s", err);
					return 0;
				}
				// BFMatcher matcher(NORM_L1, int(unique));
				// matcher.match(descriptor_1, descriptor_2, matches);
			}
			else if( strcmp(metric[0],"Hamming") == 0)
			{
				Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create("BruteForce-Hamming");
				try
				{
					matcher->knnMatch(descriptor_1, descriptor_2, matches1, 2);	
				}
				catch(Exception &e)
				{
					const char *err = e.what();
					Scierror(999, "%s", err);
					return 0;
				}
				// BFMatcher matcher(NORM_HAMMING, int(unique));
				// matcher.match(descriptor_1, descriptor_2, matches);
			}
			else if( strcmp(metric[0],"Hamming_2") == 0)
			{
				sciprint("\nInside\n");
				Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create("BruteForce-Hamming(2)");
				try
				{
					matcher->match(descriptor_1, descriptor_2, matches);
				}
				catch(Exception &e)
				{
					const char *err = e.what();
					Scierror(999, "%s", err);
					return 0;
				}
				// BFMatcher matcher(NORM_HAMMING2, int(unique));
				// matcher.match(descriptor_1, descriptor_2, matches);
			}
			else
			{
				Scierror(999,"Enter a Valid value for Metric Argument\n");
				return 0;
			}
		}
		
		else if( strcmp(method[0],"Approximate") == 0)
		{
			Mat descriptor_1(Rows_1, Cols_1, CV_32FC1);
			//descriptor_2(Rows_2, Cols_2, CV_32FC1);
			
			// Converting feature_1 vector from double matrix to cv::mat.
			for(int i=0; i<Rows_1; i++)
			{
				for(int j=0; j<Cols_1; j++)
				{
					descriptor_1.at<float>(i,j) = float(feature_1[j*Rows_1 + i]);
				}
			}
			
			// Converting feature_1 vector from double matrix to cv::mat
			for(int i=0; i<Rows_2; i++)
			{
				for(int j=0; j<Cols_2; j++)
				{
					descriptor_2.at<float>(i,j) = float(feature_2[j*Rows_2 + i]);
				}
			}
			FlannBasedMatcher matcher;
			// Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create("FlannBased");
			try
			{
				matcher.knnMatch(descriptor_1, descriptor_2, matches1, 2);	
			}
			catch(Exception &e)
			{
				const char *err = e.what();
				Scierror(999, "%s", err);
				return 0;
			}
		}
		else
		{
			Scierror(999,"Enter a Valid value for Method Argument\n");
			return 0;
		}		
		
		if(matches1.size() != 0)
		{
			for(int i = 0; i < (int)matches1.size(); i++)
			{
				if((matches1[i][0].distance < 0.7 * (matches1[i][1].distance)) && 
					((int)matches1[i].size() <= 2 && (int)matches1[i].size() > 0))
				{
					valid_m.push_back(matches1[i][0]);
				}
			}
		}

		if(matches.size() != 0)
		{
			int size = (int) matches.size();
			for(int i = 0; i < (int)matches.size(); i++)
			{	
				if((matches[i].distance < 0.7 * (matches[i + 1].distance)))
				{
					valid_m.push_back(matches[i]);
				}
			}
		}
		
		// Extracting Index Pairs from the match struct.
		indexPairs = (double*)malloc(sizeof(double) * valid_m.size() * 2);
		for( int i=0; i<valid_m.size(); i++)
		{
			indexPairs[i] = valid_m[i].queryIdx + 1;
			indexPairs[valid_m.size() + i] = valid_m[i].trainIdx + 1;
		}

		// Extracting distance vector from valid matches.
		matchMetric = (double*)malloc(sizeof(double) * valid_m.size());
		for( int i=0; i<valid_m.size(); i++)
		{
			matchMetric[i] = valid_m[i].distance;
		}		

		// Creating Output Arguments.
		sciErr = createMatrixOfDouble(pvApiCtx, nbInputArgument(pvApiCtx) + 1, valid_m.size(), 2, indexPairs);
		if(sciErr.iErr)
		{
			printError(&sciErr, 0);
			return 0;
		}
		sciErr = createMatrixOfDouble(pvApiCtx, nbInputArgument(pvApiCtx) + 2, valid_m.size(), 1, matchMetric);
		if(sciErr.iErr)
		{
			printError(&sciErr, 0);
			return 0;
		}

		// Return Arguments
		AssignOutputVariable(pvApiCtx, 1) = nbInputArgument(pvApiCtx) + 1;
		AssignOutputVariable(pvApiCtx, 2) = nbInputArgument(pvApiCtx) + 2;

		free(method);
		ReturnArguments(pvApiCtx);
		return 0;			
	}
}
