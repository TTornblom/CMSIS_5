#include "FIRF16.h"
#include <stdio.h>
#include "Error.h"

#define SNR_THRESHOLD 60

/* 

Reference patterns are generated with
a double precision computation.

*/
#define REL_ERROR (1.0e-2)

#if defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE)
static __ALIGNED(8) float16_t coeffArray[32];
#endif 

static void checkInnerTail(float16_t *b)
{
    ASSERT_TRUE(b[0] == 0.0f);
    ASSERT_TRUE(b[1] == 0.0f);
    ASSERT_TRUE(b[2] == 0.0f);
    ASSERT_TRUE(b[3] == 0.0f);
}

// Coef must be padded to a multiple of 4
#define FIRCOEFPADDING 2

    void FIRF16::test_fir_f16()
    {
        

        const int16_t *configp = configs.ptr();
        float16_t *statep = state.ptr();
        const float16_t *orgcoefsp = coefs.ptr();
        
        const float16_t *coefsp;
        const float16_t *inputp = inputs.ptr();
        float16_t *outp = output.ptr();

        unsigned long i;
#if defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE)
        int j;
#endif
        int blockSize;
        int numTaps;
        int round;

        

        /*

        Python script is generating different tests with
        different blockSize and numTaps.

        We loop on those configs.

        */
        for(i=0; i < configs.nbSamples() ; i += 2)
        {
           blockSize = configp[0];
           numTaps = configp[1];

#if defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE)
            /* Copy coefficients and pad to zero 
           */
           memset(coeffArray,127,32*sizeof(float16_t));
           round = numTaps >> FIRCOEFPADDING;
           if ((round << FIRCOEFPADDING) < numTaps)
           {
             round ++;
           }
           round = round<<FIRCOEFPADDING;
           memset(coeffArray,0,round*sizeof(float16_t));

           //printf("blockSize=%d, numTaps=%d, round=%d (%d)\n",blockSize,numTaps,round,round - numTaps);


           for(j=0;j < numTaps; j++)
           {
              coeffArray[j] = orgcoefsp[j];
           }
   
           coefsp = coeffArray;
#else
           coefsp = orgcoefsp;
#endif

           /*

           The filter is initialized with the coefs, blockSize and numTaps.

           */
           arm_fir_init_f16(&this->S,numTaps,coefsp,statep,blockSize);

           /*

           Input pointer is reset since the same input pattern is used

           */
           inputp = inputs.ptr();

           /*
           
           Python script is filtering a 2*blockSize number of samples.
           We do the same filtering in two pass to check (indirectly that
           the state management of the fir is working.)

           */

           arm_fir_f16(&this->S,inputp,outp,blockSize);
           outp += blockSize;
           checkInnerTail(outp);
           
           inputp += blockSize;
           arm_fir_f16(&this->S,inputp,outp,blockSize);
           outp += blockSize;
           checkInnerTail(outp);

           configp += 2;
           orgcoefsp += numTaps;

        }


        ASSERT_EMPTY_TAIL(output);

        ASSERT_SNR(output,ref,(float16_t)SNR_THRESHOLD);

        ASSERT_REL_ERROR(output,ref,REL_ERROR);

    } 

 
    void FIRF16::setUp(Testing::testID_t id,std::vector<Testing::param_t>& params,Client::PatternMgr *mgr)
    {
      
       (void)params;
       
       switch(id)
       {
        case FIRF16::TEST_FIR_F16_1:
        break;

       }
      

       inputs.reload(FIRF16::FIRINPUTS_F16_ID,mgr);
       coefs.reload(FIRF16::FIRCOEFS_F16_ID,mgr);
       configs.reload(FIRF16::FIRCONFIGS_S16_ID,mgr);
       ref.reload(FIRF16::FIRREFS_F16_ID,mgr);

       output.create(ref.nbSamples(),FIRF16::OUT_F16_ID,mgr);
       /* > Max  8*ceil(blockSize,8) +  blockSize + numTaps - 1 as generated by Python script */
       state.create(47+47,FIRF16::OUT_F16_ID,mgr);
    }

    void FIRF16::tearDown(Testing::testID_t id,Client::PatternMgr *mgr)
    {
        (void)id;
        output.dump(mgr);
    }
