/**
  ******************************************************************************
  * @file    stm32f4xx_hal_hash_ex.c
  * @author  MCD Application Team
  * @brief   Extended HASH HAL module driver.
  *          This file provides firmware functions to manage the following
  *          functionalities of the HASH peripheral for SHA-224 and SHA-256
  *          algorithms:
  *           + HASH or HMAC processing in polling mode
  *           + HASH or HMAC processing in interrupt mode
  *           + HASH or HMAC processing in DMA mode
  *         Additionally, this file provides functions to manage HMAC
  *         multi-buffer DMA-based processing for MD-5, SHA-1, SHA-224
  *         and SHA-256.
  *
  *
  @verbatim
  ==============================================================================
                     ##### How to use this driver #####
  ==============================================================================
    [..]
    The HASH HAL driver can be used as follows:
    (#)Initialize the HASH low level resources by implementing the HAL_HASH_MspInit():
        (##) Enable the HASH interface clock using __HAL_RCC_HASH_CLK_ENABLE()
        (##) In case of using processing APIs based on interrupts (e.g. HAL_HMACEx_SHA224_Start())
            (+++) Configure the HASH interrupt priority using HAL_NVIC_SetPriority()
            (+++) Enable the HASH IRQ handler using HAL_NVIC_EnableIRQ()
            (+++) In HASH IRQ handler, call HAL_HASH_IRQHandler()
        (##) In case of using DMA to control data transfer (e.g. HAL_HMACEx_SH224_Start_DMA())
            (+++) Enable the DMAx interface clock using __DMAx_CLK_ENABLE()
            (+++) Configure and enable one DMA stream one for managing data transfer from
                memory to peripheral (input stream). Managing data transfer from
                peripheral to memory can be performed only using CPU
            (+++) Associate the initialized DMA handle to the HASH DMA handle
                using  __HAL_LINKDMA()
            (+++) Configure the priority and enable the NVIC for the transfer complete
                interrupt on the DMA Stream: HAL_NVIC_SetPriority() and HAL_NVIC_EnableIRQ()
    (#)Initialize the HASH HAL using HAL_HASH_Init(). This function configures mainly:
        (##) The data type: 1-bit, 8-bit, 16-bit and 32-bit.
        (##) For HMAC, the encryption key.
        (##) For HMAC, the key size used for encryption.
    (#)Three processing functions are available:
        (##) Polling mode: processing APIs are blocking functions
             i.e. they process the data and wait till the digest computation is finished
             e.g. HAL_HASHEx_SHA224_Start()
        (##) Interrupt mode: encryption and decryption APIs are not blocking functions
                i.e. they process the data under interrupt
                e.g. HAL_HASHEx_SHA224_Start_IT()
        (##) DMA mode: processing APIs are not blocking functions and the CPU is
             not used for data transfer i.e. the data transfer is ensured by DMA,
                e.g. HAL_HASHEx_xxx_Start_DMA(). Note that in DMA mode, a call to
                HAL_HASHEx_xxx_Finish() is then required to retrieve the digest.

   (#)Multi-buffer processing is possible in polling, interrupt and DMA modes.
        (##) In polling mode, only multi-buffer HASH processing is possible.
             API HAL_HASHEx_xxx_Accumulate() must be called for each input buffer, except for the last one.
             User must resort to HAL_HASHEx_xxx_Accumulate_End() to enter the last one and retrieve as
             well the computed digest.

        (##) In interrupt mode, API HAL_HASHEx_xxx_Accumulate_IT() must be called for each input buffer,
             except for the last one.
             User must resort to HAL_HASHEx_xxx_Accumulate_End_IT() to enter the last one and retrieve as
             well the computed digest.

        (##) In DMA mode, multi-buffer HASH and HMAC processing are possible.

              (+++) HASH processing: once initialization is done, MDMAT bit must be set through
               __HAL_HASH_SET_MDMAT() macro.
             From that point, each buffer can be fed to the Peripheral through HAL_HASHEx_xxx_Start_DMA() API.
             Before entering the last buffer, reset the MDMAT bit with __HAL_HASH_RESET_MDMAT()
             macro then wrap-up the HASH processing in feeding the last input buffer through the
             same API HAL_HASHEx_xxx_Start_DMA(). The digest can then be retrieved with a call to
             API HAL_HASHEx_xxx_Finish().

             (+++) HMAC processing (MD-5, SHA-1, SHA-224 and SHA-256 must all resort to
             extended functions): after initialization, the key and the first input buffer are entered
             in the Peripheral with the API HAL_HMACEx_xxx_Step1_2_DMA(). This carries out HMAC step 1 and
             starts step 2.
             The following buffers are next entered with the API  HAL_HMACEx_xxx_Step2_DMA(). At this
             point, the HMAC processing is still carrying out step 2.
             Then, step 2 for the last input buffer and step 3 are carried out by a single call
             to HAL_HMACEx_xxx_Step2_3_DMA().

             The digest can finally be retrieved with a call to API HAL_HASH_xxx_Finish() for
             MD-5 and SHA-1, to HAL_HASHEx_xxx_Finish() for SHA-224 and SHA-256.


  @endverbatim
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2017 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/** @addtogroup STM32F4xx_HAL_Driver
  * @{
  */

/** @defgroup HASHEx HASHEx
  * @brief HASH Extension HAL module driver.
  * @{
  */

/** @defgroup HASHEx_Exported_Functions_Group1 HASH extended processing functions in polling mode
  *  @brief   HASH extended processing functions using polling mode.
  *
@verbatim
 ===============================================================================
               ##### Polling mode HASH extended processing functions #####
 ===============================================================================
    [..]  This section provides functions allowing to calculate in polling mode
          the hash value using one of the following algorithms:
      (+) SHA224
         (++) HAL_HASHEx_SHA224_Start()
         (++) HAL_HASHEx_SHA224_Accmlt()
         (++) HAL_HASHEx_SHA224_Accmlt_End()
      (+) SHA256
         (++) HAL_HASHEx_SHA256_Start()
         (++) HAL_HASHEx_SHA256_Accmlt()
         (++) HAL_HASHEx_SHA256_Accmlt_End()

    [..] For a single buffer to be hashed, user can resort to HAL_HASH_xxx_Start().

#if defined(STM32F437xx) || defined(STM32F439xx) || defined(STM32F479xx)

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/** @addtogroup HASHEx_Private_Functions
  * @{
  */
static void HASHEx_DMAXferCplt(DMA_HandleTypeDef* hdma);
static void HASHEx_WriteData(uint8_t* pInBuffer, uint32_t Size);
static void HASHEx_GetDigest(uint8_t* pMsgDigest, uint8_t Size);
static void HASHEx_DMAError(DMA_HandleTypeDef* hdma);
/**
  * @}
  */
HAL_StatusTypeDef HAL_HASHEx_SHA224_Start(HASH_HandleTypeDef* hhash, uint8_t* pInBuffer, uint32_t Size,
    uint8_t* pOutBuffer, uint32_t Timeout)
{
    return HASH_Start(hhash, pInBuffer, Size, pOutBuffer, Timeout, HASH_ALGOSELECTION_SHA224);
}

/** @addtogroup HASHEx_Private_Functions
  * @{
  */
HAL_StatusTypeDef HAL_HASHEx_SHA224_Accmlt(HASH_HandleTypeDef* hhash, uint8_t* pInBuffer, uint32_t Size)
{
    return HASH_Accumulate(hhash, pInBuffer, Size, HASH_ALGOSELECTION_SHA224);
}

/**
  * @brief  Writes the input buffer in data register.
  * @param  pInBuffer Pointer to input buffer
  * @param  Size The size of input buffer
  * @retval None
  */
HAL_StatusTypeDef HAL_HASHEx_SHA224_Accmlt_End(HASH_HandleTypeDef* hhash, uint8_t* pInBuffer, uint32_t Size,
    uint8_t* pOutBuffer, uint32_t Timeout)
{
    uint32_t buffercounter;
    uint32_t inputaddr = (uint32_t)pInBuffer;

    for (buffercounter = 0U; buffercounter < Size; buffercounter += 4U) {
        HASH->DIN = *(uint32_t*)inputaddr;
        inputaddr += 4U;
    }
}

/**
  * @brief  Provides the message digest result.
  * @param  pMsgDigest Pointer to the message digest
  * @param  Size The size of the message digest in bytes
  * @retval None
  */
HAL_StatusTypeDef HAL_HASHEx_SHA256_Start(HASH_HandleTypeDef* hhash, uint8_t* pInBuffer, uint32_t Size,
    uint8_t* pOutBuffer, uint32_t Timeout)
{
    uint32_t msgdigest = (uint32_t)pMsgDigest;

    switch (Size) {
    case 16U:
        /* Read the message digest */
        *(uint32_t*)(msgdigest) = __REV(HASH->HR[0U]);
        msgdigest += 4U;
        *(uint32_t*)(msgdigest) = __REV(HASH->HR[1U]);
        msgdigest += 4U;
        *(uint32_t*)(msgdigest) = __REV(HASH->HR[2U]);
        msgdigest += 4U;
        *(uint32_t*)(msgdigest) = __REV(HASH->HR[3U]);
        break;
    case 20U:
        /* Read the message digest */
        *(uint32_t*)(msgdigest) = __REV(HASH->HR[0U]);
        msgdigest += 4U;
        *(uint32_t*)(msgdigest) = __REV(HASH->HR[1U]);
        msgdigest += 4U;
        *(uint32_t*)(msgdigest) = __REV(HASH->HR[2U]);
        msgdigest += 4U;
        *(uint32_t*)(msgdigest) = __REV(HASH->HR[3U]);
        msgdigest += 4U;
        *(uint32_t*)(msgdigest) = __REV(HASH->HR[4U]);
        break;
    case 28U:
        /* Read the message digest */
        *(uint32_t*)(msgdigest) = __REV(HASH->HR[0U]);
        msgdigest += 4U;
        *(uint32_t*)(msgdigest) = __REV(HASH->HR[1U]);
        msgdigest += 4U;
        *(uint32_t*)(msgdigest) = __REV(HASH->HR[2U]);
        msgdigest += 4U;
        *(uint32_t*)(msgdigest) = __REV(HASH->HR[3U]);
        msgdigest += 4U;
        *(uint32_t*)(msgdigest) = __REV(HASH->HR[4U]);
        msgdigest += 4U;
        *(uint32_t*)(msgdigest) = __REV(HASH_DIGEST->HR[5U]);
        msgdigest += 4U;
        *(uint32_t*)(msgdigest) = __REV(HASH_DIGEST->HR[6U]);
        break;
    case 32U:
        /* Read the message digest */
        *(uint32_t*)(msgdigest) = __REV(HASH->HR[0U]);
        msgdigest += 4U;
        *(uint32_t*)(msgdigest) = __REV(HASH->HR[1U]);
        msgdigest += 4U;
        *(uint32_t*)(msgdigest) = __REV(HASH->HR[2U]);
        msgdigest += 4U;
        *(uint32_t*)(msgdigest) = __REV(HASH->HR[3U]);
        msgdigest += 4U;
        *(uint32_t*)(msgdigest) = __REV(HASH->HR[4U]);
        msgdigest += 4U;
        *(uint32_t*)(msgdigest) = __REV(HASH_DIGEST->HR[5U]);
        msgdigest += 4U;
        *(uint32_t*)(msgdigest) = __REV(HASH_DIGEST->HR[6U]);
        msgdigest += 4U;
        *(uint32_t*)(msgdigest) = __REV(HASH_DIGEST->HR[7U]);
        break;
    default:
        break;
    }
}

/**
  * @brief  DMA HASH Input Data complete callback.
  * @param  hdma DMA handle
  * @retval None
  */
static void HASHEx_DMAXferCplt(DMA_HandleTypeDef* hdma)
{
    return HASH_Accumulate(hhash, pInBuffer, Size, HASH_ALGOSELECTION_SHA256);
}

/**
  * @brief  DMA HASH communication error callback.
  * @param  hdma DMA handle
  * @retval None
  */
HAL_StatusTypeDef HAL_HASHEx_SHA256_Accmlt_End(HASH_HandleTypeDef* hhash, uint8_t* pInBuffer, uint32_t Size,
    uint8_t* pOutBuffer, uint32_t Timeout)
{
    HASH_HandleTypeDef* hhash = (HASH_HandleTypeDef*)((DMA_HandleTypeDef*)hdma)->Parent;
    hhash->State = HAL_HASH_STATE_READY;
    HAL_HASH_ErrorCallback(hhash);
}

/**
  * @}
  */

/** @defgroup HASHEx_Exported_Functions_Group2 HASH extended processing functions in interrupt mode
  *  @brief   HASH extended processing functions using interrupt mode.
  *
@verbatim
 ===============================================================================
              ##### HASH processing using polling mode functions #####
 ===============================================================================
    [..]  This section provides functions allowing to calculate in polling mode
          the hash value using one of the following algorithms:
      (+) SHA224
      (+) SHA256

@endverbatim
  * @{
  */

/**
  * @brief  Initialize the HASH peripheral in SHA224 mode, next process pInBuffer then
  *         read the computed digest in interruption mode.
  * @note   Digest is available in pOutBuffer.
  * @param  hhash HASH handle.
  * @param  pInBuffer pointer to the input buffer (buffer to be hashed).
  * @param  Size length of the input buffer in bytes.
  * @param  pOutBuffer pointer to the computed digest. Digest size is 28 bytes.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_HASHEx_SHA224_Start_IT(HASH_HandleTypeDef* hhash, uint8_t* pInBuffer, uint32_t Size,
    uint8_t* pOutBuffer)
{
    return HASH_Start_IT(hhash, pInBuffer, Size, pOutBuffer, HASH_ALGOSELECTION_SHA224);
}

/**
  * @brief  If not already done, initialize the HASH peripheral in SHA224 mode then
  *         processes pInBuffer in interruption mode.
  * @note   Consecutive calls to HAL_HASHEx_SHA224_Accmlt_IT() can be used to feed
  *         several input buffers back-to-back to the Peripheral that will yield a single
  *         HASH signature once all buffers have been entered. Wrap-up of input
  *         buffers feeding and retrieval of digest is done by a call to
  *         HAL_HASHEx_SHA224_Accmlt_End_IT().
  * @note   Field hhash->Phase of HASH handle is tested to check whether or not
  *         the Peripheral has already been initialized.
  * @note   The input buffer size (in bytes) must be a multiple of 4 otherwise, the
  *         HASH digest computation is corrupted. Only HAL_HASHEx_SHA224_Accmlt_End_IT() is able
  *         to manage the ending buffer with a length in bytes not a multiple of 4.
  * @param  hhash HASH handle.
  * @param  pInBuffer pointer to the input buffer (buffer to be hashed).
  * @param  Size length of the input buffer in bytes, must be a multiple of 4.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_HASHEx_SHA224_Accmlt_IT(HASH_HandleTypeDef* hhash, uint8_t* pInBuffer, uint32_t Size)
{
    return HASH_Accumulate_IT(hhash, pInBuffer, Size, HASH_ALGOSELECTION_SHA224);
}

/**
  * @brief  Initializes the HASH peripheral in SHA224 mode
  *         then processes pInBuffer. The digest is available in pOutBuffer
  * @param  hhash pointer to a HASH_HandleTypeDef structure that contains
  *         the configuration information for HASH module
  * @param  pInBuffer Pointer to the input buffer (buffer to be hashed).
  * @param  Size Length of the input buffer in bytes.
  *          If the Size is not multiple of 64 bytes, the padding is managed by hardware.
  * @param  pOutBuffer Pointer to the computed digest. Its size must be 28 bytes.
  * @param  Timeout Specify Timeout value
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_HASHEx_SHA224_Accmlt_End_IT(HASH_HandleTypeDef* hhash, uint8_t* pInBuffer, uint32_t Size,
    uint8_t* pOutBuffer)
{
    return HASH_Start_IT(hhash, pInBuffer, Size, pOutBuffer, HASH_ALGOSELECTION_SHA224);
}

/**
  * @brief  Initializes the HASH peripheral in SHA256 mode then processes pInBuffer.
            The digest is available in pOutBuffer.
  * @param  hhash pointer to a HASH_HandleTypeDef structure that contains
  *         the configuration information for HASH module
  * @param  pInBuffer Pointer to the input buffer (buffer to be hashed).
  * @param  Size Length of the input buffer in bytes.
  *          If the Size is not multiple of 64 bytes, the padding is managed by hardware.
  * @param  pOutBuffer Pointer to the computed digest. Its size must be 32 bytes.
  * @param  Timeout Specify Timeout value
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_HASHEx_SHA256_Start_IT(HASH_HandleTypeDef* hhash, uint8_t* pInBuffer, uint32_t Size,
    uint8_t* pOutBuffer)
{
    return HASH_Start_IT(hhash, pInBuffer, Size, pOutBuffer, HASH_ALGOSELECTION_SHA256);
}

/**
  * @brief  If not already done, initialize the HASH peripheral in SHA256 mode then
  *         processes pInBuffer in interruption mode.
  * @note   Consecutive calls to HAL_HASHEx_SHA256_Accmlt_IT() can be used to feed
  *         several input buffers back-to-back to the Peripheral that will yield a single
  *         HASH signature once all buffers have been entered. Wrap-up of input
  *         buffers feeding and retrieval of digest is done by a call to
  *         HAL_HASHEx_SHA256_Accmlt_End_IT().
  * @note   Field hhash->Phase of HASH handle is tested to check whether or not
  *         the Peripheral has already been initialized.
  * @note   The input buffer size (in bytes) must be a multiple of 4 otherwise, the
  *         HASH digest computation is corrupted. Only HAL_HASHEx_SHA256_Accmlt_End_IT() is able
  *         to manage the ending buffer with a length in bytes not a multiple of 4.
  * @param  hhash HASH handle.
  * @param  pInBuffer pointer to the input buffer (buffer to be hashed).
  * @param  Size length of the input buffer in bytes, must be a multiple of 4.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_HASHEx_SHA256_Accmlt_IT(HASH_HandleTypeDef* hhash, uint8_t* pInBuffer, uint32_t Size)
{
    return HASH_Accumulate_IT(hhash, pInBuffer, Size, HASH_ALGOSELECTION_SHA256);
}

/**
  * @brief  Initializes the HASH peripheral in SHA224 mode
  *         then processes pInBuffer. The digest is available in pOutBuffer
  * @param  hhash pointer to a HASH_HandleTypeDef structure that contains
  *         the configuration information for HASH module
  * @param  pInBuffer Pointer to the input buffer (buffer to be hashed).
  * @param  Size Length of the input buffer in bytes.
  *          If the Size is not multiple of 64 bytes, the padding is managed by hardware.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_HASHEx_SHA256_Accmlt_End_IT(HASH_HandleTypeDef* hhash, uint8_t* pInBuffer, uint32_t Size,
    uint8_t* pOutBuffer)
{
    return HASH_Start_IT(hhash, pInBuffer, Size, pOutBuffer, HASH_ALGOSELECTION_SHA256);
}

/**
  * @}
  */

/** @defgroup HASHEx_Exported_Functions_Group3 HASH extended processing functions in DMA mode
  *  @brief   HASH extended processing functions using DMA mode.
  *
@verbatim
 ===============================================================================
                ##### DMA mode HASH extended  processing functions #####
 ===============================================================================
    [..]  This section provides functions allowing to calculate in DMA mode
          the hash value using one of the following algorithms:
      (+) SHA224
         (++) HAL_HASHEx_SHA224_Start_DMA()
         (++) HAL_HASHEx_SHA224_Finish()
      (+) SHA256
         (++) HAL_HASHEx_SHA256_Start_DMA()
         (++) HAL_HASHEx_SHA256_Finish()

    [..]  When resorting to DMA mode to enter the data in the Peripheral, user must resort
          to  HAL_HASHEx_xxx_Start_DMA() then read the resulting digest with
          HAL_HASHEx_xxx_Finish().

    [..]  In case of multi-buffer HASH processing, MDMAT bit must first be set before
          the successive calls to HAL_HASHEx_xxx_Start_DMA(). Then, MDMAT bit needs to be
          reset before the last call to HAL_HASHEx_xxx_Start_DMA(). Digest is finally
          retrieved thanks to HAL_HASHEx_xxx_Finish().

@endverbatim
  * @{
  */

/**
  * @brief  Initialize the HASH peripheral in SHA224 mode then initiate a DMA transfer
  *         to feed the input buffer to the Peripheral.
  * @note   Once the DMA transfer is finished, HAL_HASHEx_SHA224_Finish() API must
  *         be called to retrieve the computed digest.
  * @param  hhash HASH handle.
  * @param  pInBuffer pointer to the input buffer (buffer to be hashed).
  * @param  Size length of the input buffer in bytes.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_HASHEx_SHA224_Start_DMA(HASH_HandleTypeDef* hhash, uint8_t* pInBuffer, uint32_t Size)
{
    return HASH_Start_DMA(hhash, pInBuffer, Size, HASH_ALGOSELECTION_SHA224);
}

/**
  * @brief  Return the computed digest in SHA224 mode.
  * @note   The API waits for DCIS to be set then reads the computed digest.
  * @note   HAL_HASHEx_SHA224_Finish() can be used as well to retrieve the digest in
  *         HMAC SHA224 mode.
  * @param  hhash HASH handle.
  * @param  pOutBuffer pointer to the computed digest. Digest size is 28 bytes.
  * @param  Timeout Timeout value.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_HASHEx_SHA224_Finish(HASH_HandleTypeDef* hhash, uint8_t* pOutBuffer, uint32_t Timeout)
{
    return HASH_Finish(hhash, pOutBuffer, Timeout);
}

/**
  * @brief  Initializes the HASH peripheral in SHA256 mode then processes pInBuffer.
            The digest is available in pOutBuffer.
  * @param  hhash pointer to a HASH_HandleTypeDef structure that contains
  *         the configuration information for HASH module
  * @param  pInBuffer Pointer to the input buffer (buffer to be hashed).
  * @param  Size Length of the input buffer in bytes.
  *          If the Size is not multiple of 64 bytes, the padding is managed by hardware.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_HASHEx_SHA256_Accumulate(HASH_HandleTypeDef* hhash, uint8_t* pInBuffer, uint32_t Size)
{
    /* Process Locked */
    __HAL_LOCK(hhash);

    /* Change the HASH state */
    hhash->State = HAL_HASH_STATE_BUSY;

    /* Check if initialization phase has already been performed */
    if (hhash->Phase == HAL_HASH_PHASE_READY) {
        /* Select the SHA256 mode and reset the HASH processor core, so that the HASH will be ready to compute
       the message digest of a new message */
        HASH->CR |= HASH_ALGOSELECTION_SHA256 | HASH_CR_INIT;
    }

    /* Set the phase */
    hhash->Phase = HAL_HASH_PHASE_PROCESS;

    /* Configure the number of valid bits in last word of the message */
    __HAL_HASH_SET_NBVALIDBITS(Size);

    /* Write input buffer in data register */
    HASHEx_WriteData(pInBuffer, Size);

    /* Change the HASH state */
    hhash->State = HAL_HASH_STATE_READY;

    /* Process Unlocked */
    __HAL_UNLOCK(hhash);

    /* Return function status */
    return HAL_OK;
}

/**
  * @brief  Return the computed digest in SHA256 mode.
  * @note   The API waits for DCIS to be set then reads the computed digest.
  * @note   HAL_HASHEx_SHA256_Finish() can be used as well to retrieve the digest in
  *         HMAC SHA256 mode.
  * @param  hhash HASH handle.
  * @param  pOutBuffer pointer to the computed digest. Digest size is 32 bytes.
  * @param  Timeout Timeout value.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_HASHEx_SHA256_Finish(HASH_HandleTypeDef* hhash, uint8_t* pOutBuffer, uint32_t Timeout)
{
    return HASH_Finish(hhash, pOutBuffer, Timeout);
}

/**
  * @}
  */

/** @defgroup HASHEx_Exported_Functions_Group4 HMAC extended processing functions in polling mode
  *  @brief   HMAC extended processing functions using polling mode.
  *
@verbatim
 ===============================================================================
             ##### Polling mode HMAC extended processing functions #####
 ===============================================================================
            ##### HMAC processing using polling mode functions #####
 ===============================================================================
    [..]  This section provides functions allowing to calculate in polling mode
          the HMAC value using one of the following algorithms:
      (+) SHA224
      (+) SHA256

@endverbatim
  * @{
  */

/**
  * @brief  Initializes the HASH peripheral in HMAC SHA224 mode
  *         then processes pInBuffer. The digest is available in pOutBuffer.
  * @param  hhash pointer to a HASH_HandleTypeDef structure that contains
  *         the configuration information for HASH module
  * @param  pInBuffer Pointer to the input buffer (buffer to be hashed).
  * @param  Size Length of the input buffer in bytes.
  *          If the Size is not multiple of 64 bytes, the padding is managed by hardware.
  * @param  pOutBuffer Pointer to the computed digest. Its size must be 20 bytes.
  * @param  Timeout Timeout value
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_HMACEx_SHA224_Start(HASH_HandleTypeDef* hhash, uint8_t* pInBuffer, uint32_t Size,
    uint8_t* pOutBuffer, uint32_t Timeout)
{
    uint32_t tickstart = 0U;

    /* Process Locked */
    __HAL_LOCK(hhash);

    /* Change the HASH state */
    hhash->State = HAL_HASH_STATE_BUSY;

    /* Check if initialization phase has already been performed */
    if (hhash->Phase == HAL_HASH_PHASE_READY) {
        /* Check if key size is greater than 64 bytes */
        if (hhash->Init.KeySize > 64U) {
            /* Select the HMAC SHA224 mode */
            HASH->CR |= (HASH_ALGOSELECTION_SHA224 | HASH_ALGOMODE_HMAC | HASH_HMAC_KEYTYPE_LONGKEY | HASH_CR_INIT);
        } else {
            /* Select the HMAC SHA224 mode */
            HASH->CR |= (HASH_ALGOSELECTION_SHA224 | HASH_ALGOMODE_HMAC | HASH_CR_INIT);
        }
    }

    /* Set the phase */
    hhash->Phase = HAL_HASH_PHASE_PROCESS;

    /************************** STEP 1 ******************************************/
    /* Configure the number of valid bits in last word of the message */
    __HAL_HASH_SET_NBVALIDBITS(hhash->Init.KeySize);

    /* Write input buffer in data register */
    HASHEx_WriteData(hhash->Init.pKey, hhash->Init.KeySize);

    /* Start the digest calculation */
    __HAL_HASH_START_DIGEST();

    /* Get tick */
    tickstart = HAL_GetTick();

    while ((HASH->SR & HASH_FLAG_BUSY) == HASH_FLAG_BUSY) {
        /* Check for the Timeout */
        if (Timeout != HAL_MAX_DELAY) {
            if ((Timeout == 0U) || ((HAL_GetTick() - tickstart) > Timeout)) {
                /* Change state */
                hhash->State = HAL_HASH_STATE_TIMEOUT;

                /* Process Unlocked */
                __HAL_UNLOCK(hhash);

                return HAL_TIMEOUT;
            }
        }
    }
    /************************** STEP 2 ******************************************/
    /* Configure the number of valid bits in last word of the message */
    __HAL_HASH_SET_NBVALIDBITS(Size);

    /* Write input buffer in data register */
    HASHEx_WriteData(pInBuffer, Size);

    /* Start the digest calculation */
    __HAL_HASH_START_DIGEST();

    /* Get tick */
    tickstart = HAL_GetTick();

    while ((HASH->SR & HASH_FLAG_BUSY) == HASH_FLAG_BUSY) {
        /* Check for the Timeout */
        if (Timeout != HAL_MAX_DELAY) {
            if ((HAL_GetTick() - tickstart) > Timeout) {
                /* Change state */
                hhash->State = HAL_HASH_STATE_TIMEOUT;

                /* Process Unlocked */
                __HAL_UNLOCK(hhash);

                return HAL_TIMEOUT;
            }
        }
    }
    /************************** STEP 3 ******************************************/
    /* Configure the number of valid bits in last word of the message */
    __HAL_HASH_SET_NBVALIDBITS(hhash->Init.KeySize);

    /* Write input buffer in data register */
    HASHEx_WriteData(hhash->Init.pKey, hhash->Init.KeySize);

    /* Start the digest calculation */
    __HAL_HASH_START_DIGEST();

    /* Get tick */
    tickstart = HAL_GetTick();

    while ((HASH->SR & HASH_FLAG_BUSY) == HASH_FLAG_BUSY) {
        /* Check for the Timeout */
        if (Timeout != HAL_MAX_DELAY) {
            if ((HAL_GetTick() - tickstart) > Timeout) {
                /* Change state */
                hhash->State = HAL_HASH_STATE_TIMEOUT;

                /* Process Unlocked */
                __HAL_UNLOCK(hhash);

                return HAL_TIMEOUT;
            }
        }
    }
    /* Read the message digest */
    HASHEx_GetDigest(pOutBuffer, 28U);

    /* Change the HASH state */
    hhash->State = HAL_HASH_STATE_READY;

    /* Process Unlocked */
    __HAL_UNLOCK(hhash);

    /* Return function status */
    return HAL_OK;
}

/**
  * @brief  Initializes the HASH peripheral in HMAC SHA256 mode
  *         then processes pInBuffer. The digest is available in pOutBuffer
  * @param  hhash pointer to a HASH_HandleTypeDef structure that contains
  *         the configuration information for HASH module
  * @param  pInBuffer Pointer to the input buffer (buffer to be hashed).
  * @param  Size Length of the input buffer in bytes.
  *          If the Size is not multiple of 64 bytes, the padding is managed by hardware.
  * @param  pOutBuffer Pointer to the computed digest. Its size must be 20 bytes.
  * @param  Timeout Timeout value
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_HMACEx_SHA256_Start(HASH_HandleTypeDef* hhash, uint8_t* pInBuffer, uint32_t Size,
    uint8_t* pOutBuffer, uint32_t Timeout)
{
    uint32_t tickstart = 0U;

    /* Process Locked */
    __HAL_LOCK(hhash);

    /* Change the HASH state */
    hhash->State = HAL_HASH_STATE_BUSY;

    /* Check if initialization phase has already been performed */
    if (hhash->Phase == HAL_HASH_PHASE_READY) {
        /* Check if key size is greater than 64 bytes */
        if (hhash->Init.KeySize > 64U) {
            /* Select the HMAC SHA256 mode */
            HASH->CR |= (HASH_ALGOSELECTION_SHA256 | HASH_ALGOMODE_HMAC | HASH_HMAC_KEYTYPE_LONGKEY);
        } else {
            /* Select the HMAC SHA256 mode */
            HASH->CR |= (HASH_ALGOSELECTION_SHA256 | HASH_ALGOMODE_HMAC);
        }
        /* Reset the HASH processor core, so that the HASH will be ready to compute
       the message digest of a new message */
        HASH->CR |= HASH_CR_INIT;
    }

    /* Set the phase */
    hhash->Phase = HAL_HASH_PHASE_PROCESS;

    /************************** STEP 1 ******************************************/
    /* Configure the number of valid bits in last word of the message */
    __HAL_HASH_SET_NBVALIDBITS(hhash->Init.KeySize);

    /* Write input buffer in data register */
    HASHEx_WriteData(hhash->Init.pKey, hhash->Init.KeySize);

    /* Start the digest calculation */
    __HAL_HASH_START_DIGEST();

    /* Get tick */
    tickstart = HAL_GetTick();

    while ((HASH->SR & HASH_FLAG_BUSY) == HASH_FLAG_BUSY) {
        /* Check for the Timeout */
        if (Timeout != HAL_MAX_DELAY) {
            if ((Timeout == 0U) || ((HAL_GetTick() - tickstart) > Timeout)) {
                /* Change state */
                hhash->State = HAL_HASH_STATE_TIMEOUT;

                /* Process Unlocked */
                __HAL_UNLOCK(hhash);

                return HAL_TIMEOUT;
            }
        }
    }
    /************************** STEP 2 ******************************************/
    /* Configure the number of valid bits in last word of the message */
    __HAL_HASH_SET_NBVALIDBITS(Size);

    /* Write input buffer in data register */
    HASHEx_WriteData(pInBuffer, Size);

    /* Start the digest calculation */
    __HAL_HASH_START_DIGEST();

    /* Get tick */
    tickstart = HAL_GetTick();

    while ((HASH->SR & HASH_FLAG_BUSY) == HASH_FLAG_BUSY) {
        /* Check for the Timeout */
        if (Timeout != HAL_MAX_DELAY) {
            if ((HAL_GetTick() - tickstart) > Timeout) {
                /* Change state */
                hhash->State = HAL_HASH_STATE_TIMEOUT;

                /* Process Unlocked */
                __HAL_UNLOCK(hhash);

                return HAL_TIMEOUT;
            }
        }
    }
    /************************** STEP 3 ******************************************/
    /* Configure the number of valid bits in last word of the message */
    __HAL_HASH_SET_NBVALIDBITS(hhash->Init.KeySize);

    /* Write input buffer in data register */
    HASHEx_WriteData(hhash->Init.pKey, hhash->Init.KeySize);

    /* Start the digest calculation */
    __HAL_HASH_START_DIGEST();

    /* Get tick */
    tickstart = HAL_GetTick();

    while ((HASH->SR & HASH_FLAG_BUSY) == HASH_FLAG_BUSY) {
        /* Check for the Timeout */
        if (Timeout != HAL_MAX_DELAY) {
            if ((HAL_GetTick() - tickstart) > Timeout) {
                /* Change state */
                hhash->State = HAL_HASH_STATE_TIMEOUT;

                /* Process Unlocked */
                __HAL_UNLOCK(hhash);

                return HAL_TIMEOUT;
            }
        }
    }
    /* Read the message digest */
    HASHEx_GetDigest(pOutBuffer, 32U);

    /* Change the HASH state */
    hhash->State = HAL_HASH_STATE_READY;

    /* Process Unlocked */
    __HAL_UNLOCK(hhash);

    /* Return function status */
    return HAL_OK;
}

/**
  * @}
  */

/** @defgroup HASHEx_Exported_Functions_Group5 HMAC extended processing functions in interrupt mode
  *  @brief   HMAC extended processing functions using interruption mode.
  *
@verbatim
 ===============================================================================
             ##### Interrupt mode HMAC extended processing functions #####
 ===============================================================================
              ##### HASH processing using interrupt functions #####
 ===============================================================================
    [..]  This section provides functions allowing to calculate in interrupt mode
          the hash value using one of the following algorithms:
      (+) SHA224
      (+) SHA256

@endverbatim
  * @{
  */

/**
  * @brief  Initialize the HASH peripheral in HMAC SHA224 mode, next process pInBuffer then
  *         read the computed digest in interrupt mode.
  * @note   Digest is available in pOutBuffer.
  * @note   Same key is used for the inner and the outer hash functions; pointer to key and
  *         key size are respectively stored in hhash->Init.pKey and hhash->Init.KeySize.
  * @param  hhash HASH handle.
  * @param  pInBuffer pointer to the input buffer (buffer to be hashed).
  * @param  Size length of the input buffer in bytes.
  * @param  pOutBuffer pointer to the computed digest. Digest size is 28 bytes.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_HMACEx_SHA224_Start_IT(HASH_HandleTypeDef* hhash, uint8_t* pInBuffer, uint32_t Size,
    uint8_t* pOutBuffer)
{
    return HMAC_Start_IT(hhash, pInBuffer, Size, pOutBuffer, HASH_ALGOSELECTION_SHA224);
}

/**
  * @brief  Initializes the HASH peripheral in SHA224 mode then processes pInBuffer.
  *         The digest is available in pOutBuffer.
  * @param  hhash pointer to a HASH_HandleTypeDef structure that contains
  *         the configuration information for HASH module
  * @param  pInBuffer Pointer to the input buffer (buffer to be hashed).
  * @param  Size Length of the input buffer in bytes.
  *          If the Size is not multiple of 64 bytes, the padding is managed by hardware.
  * @param  pOutBuffer Pointer to the computed digest. Its size must be 20 bytes.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_HMACEx_SHA256_Start_IT(HASH_HandleTypeDef* hhash, uint8_t* pInBuffer, uint32_t Size,
    uint8_t* pOutBuffer)
{
    uint32_t inputaddr;
    uint32_t buffercounter;
    uint32_t inputcounter;

    /* Process Locked */
    __HAL_LOCK(hhash);

    if (hhash->State == HAL_HASH_STATE_READY) {
        /* Change the HASH state */
        hhash->State = HAL_HASH_STATE_BUSY;

        hhash->HashInCount = Size;
        hhash->pHashInBuffPtr = pInBuffer;
        hhash->pHashOutBuffPtr = pOutBuffer;

        /* Check if initialization phase has already been performed */
        if (hhash->Phase == HAL_HASH_PHASE_READY) {
            /* Select the SHA224 mode */
            HASH->CR |= HASH_ALGOSELECTION_SHA224;
            /* Reset the HASH processor core, so that the HASH will be ready to compute
         the message digest of a new message */
            HASH->CR |= HASH_CR_INIT;
        }
        /* Reset interrupt counter */
        hhash->HashITCounter = 0U;

        /* Set the phase */
        hhash->Phase = HAL_HASH_PHASE_PROCESS;

        /* Process Unlocked */
        __HAL_UNLOCK(hhash);

        /* Enable Interrupts */
        HASH->IMR = (HASH_IT_DINI | HASH_IT_DCI);

        /* Return function status */
        return HAL_OK;
    }
    if (__HAL_HASH_GET_FLAG(HASH_FLAG_DCIS)) {
        /* Read the message digest */
        HASHEx_GetDigest(hhash->pHashOutBuffPtr, 28U);
        if (hhash->HashInCount == 0U) {
            /* Disable Interrupts */
            HASH->IMR = 0U;
            /* Change the HASH state */
            hhash->State = HAL_HASH_STATE_READY;
            /* Call digest computation complete callback */
            HAL_HASH_DgstCpltCallback(hhash);

            /* Process Unlocked */
            __HAL_UNLOCK(hhash);

            /* Return function status */
            return HAL_OK;
        }
    }
    if (__HAL_HASH_GET_FLAG(HASH_FLAG_DINIS)) {
        if (hhash->HashInCount >= 68U) {
            inputaddr = (uint32_t)hhash->pHashInBuffPtr;
            /* Write the Input block in the Data IN register */
            for (buffercounter = 0U; buffercounter < 64U; buffercounter += 4U) {
                HASH->DIN = *(uint32_t*)inputaddr;
                inputaddr += 4U;
            }
            if (hhash->HashITCounter == 0U) {
                HASH->DIN = *(uint32_t*)inputaddr;

                if (hhash->HashInCount >= 68U) {
                    /* Decrement buffer counter */
                    hhash->HashInCount -= 68U;
                    hhash->pHashInBuffPtr += 68U;
                } else {
                    hhash->HashInCount = 0U;
                    hhash->pHashInBuffPtr += hhash->HashInCount;
                }
                /* Set Interrupt counter */
                hhash->HashITCounter = 1U;
            } else {
                /* Decrement buffer counter */
                hhash->HashInCount -= 64U;
                hhash->pHashInBuffPtr += 64U;
            }
        } else {
            /* Get the buffer address */
            inputaddr = (uint32_t)hhash->pHashInBuffPtr;
            /* Get the buffer counter */
            inputcounter = hhash->HashInCount;
            /* Disable Interrupts */
            HASH->IMR &= ~(HASH_IT_DINI);
            /* Configure the number of valid bits in last word of the message */
            __HAL_HASH_SET_NBVALIDBITS(inputcounter);

            if ((inputcounter > 4U) && (inputcounter % 4U)) {
                inputcounter = (inputcounter + 4U - inputcounter % 4U);
            } else if ((inputcounter < 4U) && (inputcounter != 0U)) {
                inputcounter = 4U;
            }
            /* Write the Input block in the Data IN register */
            for (buffercounter = 0U; buffercounter < inputcounter / 4U; buffercounter++) {
                HASH->DIN = *(uint32_t*)inputaddr;
                inputaddr += 4U;
            }
            /* Start the digest calculation */
            __HAL_HASH_START_DIGEST();
            /* Reset buffer counter */
            hhash->HashInCount = 0U;
            /* Call Input data transfer complete callback */
            HAL_HASH_InCpltCallback(hhash);
        }
    }

    /* Process Unlocked */
    __HAL_UNLOCK(hhash);

    /* Return function status */
    return HAL_OK;
}

/**
  * @}
  */

/** @defgroup HASHEx_Exported_Functions_Group6 HMAC extended processing functions in DMA mode
  *  @brief   HMAC extended processing functions using DMA mode.
  *
@verbatim
 ===============================================================================
              ##### DMA mode HMAC extended processing functions #####
 ===============================================================================
    [..]  This section provides functions allowing to calculate in DMA mode
          the HMAC value using one of the following algorithms:
      (+) SHA224
         (++) HAL_HMACEx_SHA224_Start_DMA()
      (+) SHA256
         (++) HAL_HMACEx_SHA256_Start_DMA()

    [..]  When resorting to DMA mode to enter the data in the Peripheral for HMAC processing,
          user must resort to  HAL_HMACEx_xxx_Start_DMA() then read the resulting digest
          with HAL_HASHEx_xxx_Finish().


@endverbatim
  * @{
  */

/**
  * @brief  Initialize the HASH peripheral in HMAC SHA224 mode then initiate the required
  *         DMA transfers to feed the key and the input buffer to the Peripheral.
  * @note   Once the DMA transfers are finished (indicated by hhash->State set back
  *         to HAL_HASH_STATE_READY), HAL_HASHEx_SHA224_Finish() API must be called to retrieve
  *         the computed digest.
  * @note   Same key is used for the inner and the outer hash functions; pointer to key and
  *         key size are respectively stored in hhash->Init.pKey and hhash->Init.KeySize.
  * @note   If MDMAT bit is set before calling this function (multi-buffer
  *          HASH processing case), the input buffer size (in bytes) must be
  *          a multiple of 4 otherwise, the HASH digest computation is corrupted.
  *          For the processing of the last buffer of the thread, MDMAT bit must
  *          be reset and the buffer length (in bytes) doesn't have to be a
  *          multiple of 4.
  * @param  hhash HASH handle.
  * @param  pInBuffer pointer to the input buffer (buffer to be hashed).
  * @param  Size length of the input buffer in bytes.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_HASHEx_SHA256_Start_IT(HASH_HandleTypeDef* hhash, uint8_t* pInBuffer, uint32_t Size, uint8_t* pOutBuffer)
{
    uint32_t inputaddr;
    uint32_t buffercounter;
    uint32_t inputcounter;

    /* Process Locked */
    __HAL_LOCK(hhash);

    if (hhash->State == HAL_HASH_STATE_READY) {
        /* Change the HASH state */
        hhash->State = HAL_HASH_STATE_BUSY;

        hhash->HashInCount = Size;
        hhash->pHashInBuffPtr = pInBuffer;
        hhash->pHashOutBuffPtr = pOutBuffer;

        /* Check if initialization phase has already been performed */
        if (hhash->Phase == HAL_HASH_PHASE_READY) {
            /* Select the SHA256 mode */
            HASH->CR |= HASH_ALGOSELECTION_SHA256;
            /* Reset the HASH processor core, so that the HASH will be ready to compute
         the message digest of a new message */
            HASH->CR |= HASH_CR_INIT;
        }
        /* Reset interrupt counter */
        hhash->HashITCounter = 0U;

        /* Set the phase */
        hhash->Phase = HAL_HASH_PHASE_PROCESS;

        /* Process Unlocked */
        __HAL_UNLOCK(hhash);

        /* Enable Interrupts */
        HASH->IMR = (HASH_IT_DINI | HASH_IT_DCI);

        /* Return function status */
        return HAL_OK;
    }
    if (__HAL_HASH_GET_FLAG(HASH_FLAG_DCIS)) {
        /* Read the message digest */
        HASHEx_GetDigest(hhash->pHashOutBuffPtr, 32U);
        if (hhash->HashInCount == 0U) {
            /* Disable Interrupts */
            HASH->IMR = 0U;
            /* Change the HASH state */
            hhash->State = HAL_HASH_STATE_READY;
            /* Call digest computation complete callback */
            HAL_HASH_DgstCpltCallback(hhash);

            /* Process Unlocked */
            __HAL_UNLOCK(hhash);

            /* Return function status */
            return HAL_OK;
        }
    }
    if (__HAL_HASH_GET_FLAG(HASH_FLAG_DINIS)) {
        if (hhash->HashInCount >= 68U) {
            inputaddr = (uint32_t)hhash->pHashInBuffPtr;
            /* Write the Input block in the Data IN register */
            for (buffercounter = 0U; buffercounter < 64U; buffercounter += 4U) {
                HASH->DIN = *(uint32_t*)inputaddr;
                inputaddr += 4U;
            }
            if (hhash->HashITCounter == 0U) {
                HASH->DIN = *(uint32_t*)inputaddr;

                if (hhash->HashInCount >= 68U) {
                    /* Decrement buffer counter */
                    hhash->HashInCount -= 68U;
                    hhash->pHashInBuffPtr += 68U;
                } else {
                    hhash->HashInCount = 0U;
                    hhash->pHashInBuffPtr += hhash->HashInCount;
                }
                /* Set Interrupt counter */
                hhash->HashITCounter = 1U;
            } else {
                /* Decrement buffer counter */
                hhash->HashInCount -= 64U;
                hhash->pHashInBuffPtr += 64U;
            }
        } else {
            /* Get the buffer address */
            inputaddr = (uint32_t)hhash->pHashInBuffPtr;
            /* Get the buffer counter */
            inputcounter = hhash->HashInCount;
            /* Disable Interrupts */
            HASH->IMR &= ~(HASH_IT_DINI);
            /* Configure the number of valid bits in last word of the message */
            __HAL_HASH_SET_NBVALIDBITS(inputcounter);

            if ((inputcounter > 4U) && (inputcounter % 4U)) {
                inputcounter = (inputcounter + 4U - inputcounter % 4U);
            } else if ((inputcounter < 4U) && (inputcounter != 0U)) {
                inputcounter = 4U;
            }
            /* Write the Input block in the Data IN register */
            for (buffercounter = 0U; buffercounter < inputcounter / 4U; buffercounter++) {
                HASH->DIN = *(uint32_t*)inputaddr;
                inputaddr += 4U;
            }
            /* Start the digest calculation */
            __HAL_HASH_START_DIGEST();
            /* Reset buffer counter */
            hhash->HashInCount = 0U;
            /* Call Input data transfer complete callback */
            HAL_HASH_InCpltCallback(hhash);
        }
    }

    /* Process Unlocked */
    __HAL_UNLOCK(hhash);

    /* Return function status */
    return HAL_OK;
}

/**
  * @brief This function handles HASH interrupt request.
  * @param  hhash pointer to a HASH_HandleTypeDef structure that contains
  *         the configuration information for HASH module
  * @retval None
  */
void HAL_HASHEx_IRQHandler(HASH_HandleTypeDef* hhash)
{
    switch (HASH->CR & HASH_CR_ALGO) {

    case HASH_ALGOSELECTION_SHA224:
        HAL_HASHEx_SHA224_Start_IT(hhash, NULL, 0U, NULL);
        break;

    case HASH_ALGOSELECTION_SHA256:
        HAL_HASHEx_SHA256_Start_IT(hhash, NULL, 0U, NULL);
        break;

    default:
        break;
    }
}

/**
  * @}
  */

/** @defgroup HASHEx_Exported_Functions_Group7 Multi-buffer HMAC extended processing functions in DMA mode
  *  @brief   HMAC extended processing functions in multi-buffer DMA mode.
  *
@verbatim
 ===============================================================================
      ##### Multi-buffer DMA mode HMAC extended processing functions #####
 ===============================================================================
                ##### HASH processing using DMA functions #####
 ===============================================================================
    [..]  This section provides functions allowing to calculate in DMA mode
          the hash value using one of the following algorithms:
      (+) SHA224
      (+) SHA256

@endverbatim
  * @{
  */

/**
  * @brief  Initializes the HASH peripheral in SHA224 mode then enables DMA to
            control data transfer. Use HAL_HASH_SHA224_Finish() to get the digest.
  * @param  hhash pointer to a HASH_HandleTypeDef structure that contains
  *         the configuration information for HASH module
  * @param  pInBuffer Pointer to the input buffer (buffer to be hashed).
  * @param  Size Length of the input buffer in bytes.
  *          If the Size is not multiple of 64 bytes, the padding is managed by hardware.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_HASHEx_SHA224_Start_DMA(HASH_HandleTypeDef* hhash, uint8_t* pInBuffer, uint32_t Size)
{
    uint32_t inputaddr = (uint32_t)pInBuffer;

    /* Process Locked */
    __HAL_LOCK(hhash);

    /* Change the HASH state */
    hhash->State = HAL_HASH_STATE_BUSY;

    /* Check if initialization phase has already been performed */
    if (hhash->Phase == HAL_HASH_PHASE_READY) {
        /* Select the SHA224 mode and reset the HASH processor core, so that the HASH will be ready to compute
       the message digest of a new message */
        HASH->CR |= HASH_ALGOSELECTION_SHA224 | HASH_CR_INIT;
    }

    /* Configure the number of valid bits in last word of the message */
    __HAL_HASH_SET_NBVALIDBITS(Size);

    /* Set the phase */
    hhash->Phase = HAL_HASH_PHASE_PROCESS;

    /* Set the HASH DMA transfer complete callback */
    hhash->hdmain->XferCpltCallback = HASHEx_DMAXferCplt;
    /* Set the DMA error callback */
    hhash->hdmain->XferErrorCallback = HASHEx_DMAError;

    /* Enable the DMA In DMA Stream */
    HAL_DMA_Start_IT(hhash->hdmain, inputaddr, (uint32_t)&HASH->DIN, (Size % 4U ? (Size + 3U) / 4U : Size / 4U));

    /* Enable DMA requests */
    HASH->CR |= (HASH_CR_DMAE);

    /* Process Unlocked */
    __HAL_UNLOCK(hhash);

    /* Return function status */
    return HAL_OK;
}

/**
  * @brief  Returns the computed digest in SHA224
  * @param  hhash pointer to a HASH_HandleTypeDef structure that contains
  *         the configuration information for HASH module
  * @param  pOutBuffer Pointer to the computed digest. Its size must be 28 bytes.
  * @param  Timeout Timeout value
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_HASHEx_SHA224_Finish(HASH_HandleTypeDef* hhash, uint8_t* pOutBuffer, uint32_t Timeout)
{
    uint32_t tickstart = 0U;

    /* Process Locked */
    __HAL_LOCK(hhash);

    /* Change HASH peripheral state */
    hhash->State = HAL_HASH_STATE_BUSY;

    /* Get tick */
    tickstart = HAL_GetTick();

    while (HAL_IS_BIT_CLR(HASH->SR, HASH_FLAG_DCIS)) {
        /* Check for the Timeout */
        if (Timeout != HAL_MAX_DELAY) {
            if ((Timeout == 0U) || ((HAL_GetTick() - tickstart) > Timeout)) {
                /* Change state */
                hhash->State = HAL_HASH_STATE_TIMEOUT;

                /* Process Unlocked */
                __HAL_UNLOCK(hhash);

                return HAL_TIMEOUT;
            }
        }
    }

    /* Read the message digest */
    HASHEx_GetDigest(pOutBuffer, 28U);

    /* Change HASH peripheral state */
    hhash->State = HAL_HASH_STATE_READY;

    /* Process Unlocked */
    __HAL_UNLOCK(hhash);

    /* Return function status */
    return HAL_OK;
}

/**
  * @brief  Initializes the HASH peripheral in SHA256 mode then enables DMA to
            control data transfer. Use HAL_HASH_SHA256_Finish() to get the digest.
  * @param  hhash pointer to a HASH_HandleTypeDef structure that contains
  *         the configuration information for HASH module
  * @param  pInBuffer Pointer to the input buffer (buffer to be hashed).
  * @param  Size Length of the input buffer in bytes.
  *          If the Size is not multiple of 64 bytes, the padding is managed by hardware.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_HASHEx_SHA256_Start_DMA(HASH_HandleTypeDef* hhash, uint8_t* pInBuffer, uint32_t Size)
{
    uint32_t inputaddr = (uint32_t)pInBuffer;

    /* Process Locked */
    __HAL_LOCK(hhash);

    /* Change the HASH state */
    hhash->State = HAL_HASH_STATE_BUSY;

    /* Check if initialization phase has already been performed */
    if (hhash->Phase == HAL_HASH_PHASE_READY) {
        /* Select the SHA256 mode and reset the HASH processor core, so that the HASH will be ready to compute
       the message digest of a new message */
        HASH->CR |= HASH_ALGOSELECTION_SHA256 | HASH_CR_INIT;
    }

    /* Configure the number of valid bits in last word of the message */
    __HAL_HASH_SET_NBVALIDBITS(Size);

    /* Set the phase */
    hhash->Phase = HAL_HASH_PHASE_PROCESS;

    /* Set the HASH DMA transfer complete callback */
    hhash->hdmain->XferCpltCallback = HASHEx_DMAXferCplt;
    /* Set the DMA error callback */
    hhash->hdmain->XferErrorCallback = HASHEx_DMAError;

    /* Enable the DMA In DMA Stream */
    HAL_DMA_Start_IT(hhash->hdmain, inputaddr, (uint32_t)&HASH->DIN, (Size % 4U ? (Size + 3U) / 4U : Size / 4U));

    /* Enable DMA requests */
    HASH->CR |= (HASH_CR_DMAE);

    /* Process UnLock */
    __HAL_UNLOCK(hhash);

    /* Return function status */
    return HAL_OK;
}

/**
  * @brief  Returns the computed digest in SHA256.
  * @param  hhash pointer to a HASH_HandleTypeDef structure that contains
  *         the configuration information for HASH module
  * @param  pOutBuffer Pointer to the computed digest. Its size must be 32 bytes.
  * @param  Timeout Timeout value
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_HASHEx_SHA256_Finish(HASH_HandleTypeDef* hhash, uint8_t* pOutBuffer, uint32_t Timeout)
{
    uint32_t tickstart = 0U;

    /* Process Locked */
    __HAL_LOCK(hhash);

    /* Change HASH peripheral state */
    hhash->State = HAL_HASH_STATE_BUSY;

    /* Get tick */
    tickstart = HAL_GetTick();

    while (HAL_IS_BIT_CLR(HASH->SR, HASH_FLAG_DCIS)) {
        /* Check for the Timeout */
        if (Timeout != HAL_MAX_DELAY) {
            if ((Timeout == 0U) || ((HAL_GetTick() - tickstart) > Timeout)) {
                /* Change state */
                hhash->State = HAL_HASH_STATE_TIMEOUT;

                /* Process Unlocked */
                __HAL_UNLOCK(hhash);

                return HAL_TIMEOUT;
            }
        }
    }

    /* Read the message digest */
    HASHEx_GetDigest(pOutBuffer, 32U);

    /* Change HASH peripheral state */
    hhash->State = HAL_HASH_STATE_READY;

    /* Process Unlocked */
    __HAL_UNLOCK(hhash);

    /* Return function status */
    return HAL_OK;
}

/**
  * @}
  */
/** @defgroup HASHEx_Group5 HMAC processing functions using DMA mode
 *  @brief   HMAC processing functions using DMA mode .
 *
@verbatim
 ===============================================================================
                ##### HMAC processing using DMA functions #####
 ===============================================================================
    [..]  This section provides functions allowing to calculate in DMA mode
          the HMAC value using one of the following algorithms:
      (+) SHA224
      (+) SHA256

@endverbatim
  * @{
  */

/**
  * @brief  Initializes the HASH peripheral in HMAC SHA224 mode
  *         then enables DMA to control data transfer.
  * @param  hhash pointer to a HASH_HandleTypeDef structure that contains
  *         the configuration information for HASH module
  * @param  pInBuffer Pointer to the input buffer (buffer to be hashed).
  * @param  Size Length of the input buffer in bytes.
  *          If the Size is not multiple of 64 bytes, the padding is managed by hardware.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_HMACEx_SHA224_Start_DMA(HASH_HandleTypeDef* hhash, uint8_t* pInBuffer, uint32_t Size)
{
    uint32_t inputaddr;

    /* Process Locked */
    __HAL_LOCK(hhash);

    /* Change the HASH state */
    hhash->State = HAL_HASH_STATE_BUSY;

    /* Save buffer pointer and size in handle */
    hhash->pHashInBuffPtr = pInBuffer;
    hhash->HashBuffSize = Size;
    hhash->HashInCount = 0U;

    /* Check if initialization phase has already been performed */
    if (hhash->Phase == HAL_HASH_PHASE_READY) {
        /* Check if key size is greater than 64 bytes */
        if (hhash->Init.KeySize > 64U) {
            /* Select the HMAC SHA224 mode */
            HASH->CR |= (HASH_ALGOSELECTION_SHA224 | HASH_ALGOMODE_HMAC | HASH_HMAC_KEYTYPE_LONGKEY | HASH_CR_INIT);
        } else {
            /* Select the HMAC SHA224 mode */
            HASH->CR |= (HASH_ALGOSELECTION_SHA224 | HASH_ALGOMODE_HMAC | HASH_CR_INIT);
        }
    }

    /* Set the phase */
    hhash->Phase = HAL_HASH_PHASE_PROCESS;

    /* Configure the number of valid bits in last word of the message */
    __HAL_HASH_SET_NBVALIDBITS(hhash->Init.KeySize);

    /* Get the key address */
    inputaddr = (uint32_t)(hhash->Init.pKey);

    /* Set the HASH DMA transfer complete callback */
    hhash->hdmain->XferCpltCallback = HASHEx_DMAXferCplt;
    /* Set the DMA error callback */
    hhash->hdmain->XferErrorCallback = HASHEx_DMAError;

    /* Enable the DMA In DMA Stream */
    HAL_DMA_Start_IT(hhash->hdmain, inputaddr, (uint32_t)&HASH->DIN, (hhash->Init.KeySize % 4U ? (hhash->Init.KeySize + 3U) / 4U : hhash->Init.KeySize / 4U));
    /* Enable DMA requests */
    HASH->CR |= (HASH_CR_DMAE);

    /* Process Unlocked */
    __HAL_UNLOCK(hhash);

    /* Return function status */
    return HAL_OK;
}

/**
  * @brief  Initializes the HASH peripheral in HMAC SHA256 mode
  *         then enables DMA to control data transfer.
  * @param  hhash pointer to a HASH_HandleTypeDef structure that contains
  *         the configuration information for HASH module
  * @param  pInBuffer Pointer to the input buffer (buffer to be hashed).
  * @param  Size Length of the input buffer in bytes.
  *          If the Size is not multiple of 64 bytes, the padding is managed by hardware.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_HMACEx_SHA256_Start_DMA(HASH_HandleTypeDef* hhash, uint8_t* pInBuffer, uint32_t Size)
{
    uint32_t inputaddr;

    /* Process Locked */
    __HAL_LOCK(hhash);

    /* Change the HASH state */
    hhash->State = HAL_HASH_STATE_BUSY;

    /* Save buffer pointer and size in handle */
    hhash->pHashInBuffPtr = pInBuffer;
    hhash->HashBuffSize = Size;
    hhash->HashInCount = 0U;

    /* Check if initialization phase has already been performed */
    if (hhash->Phase == HAL_HASH_PHASE_READY) {
        /* Check if key size is greater than 64 bytes */
        if (hhash->Init.KeySize > 64U) {
            /* Select the HMAC SHA256 mode */
            HASH->CR |= (HASH_ALGOSELECTION_SHA256 | HASH_ALGOMODE_HMAC | HASH_HMAC_KEYTYPE_LONGKEY);
        } else {
            /* Select the HMAC SHA256 mode */
            HASH->CR |= (HASH_ALGOSELECTION_SHA256 | HASH_ALGOMODE_HMAC);
        }
        /* Reset the HASH processor core, so that the HASH will be ready to compute
       the message digest of a new message */
        HASH->CR |= HASH_CR_INIT;
    }

    /* Set the phase */
    hhash->Phase = HAL_HASH_PHASE_PROCESS;

    /* Configure the number of valid bits in last word of the message */
    __HAL_HASH_SET_NBVALIDBITS(hhash->Init.KeySize);

    /* Get the key address */
    inputaddr = (uint32_t)(hhash->Init.pKey);

    /* Set the HASH DMA transfer complete callback */
    hhash->hdmain->XferCpltCallback = HASHEx_DMAXferCplt;
    /* Set the DMA error callback */
    hhash->hdmain->XferErrorCallback = HASHEx_DMAError;

    /* Enable the DMA In DMA Stream */
    HAL_DMA_Start_IT(hhash->hdmain, inputaddr, (uint32_t)&HASH->DIN, (hhash->Init.KeySize % 4U ? (hhash->Init.KeySize + 3U) / 4U : hhash->Init.KeySize / 4U));
    /* Enable DMA requests */
    HASH->CR |= (HASH_CR_DMAE);

    /* Process Unlocked */
    __HAL_UNLOCK(hhash);

    /* Return function status */
    return HAL_OK;
}

/**
  * @}
  */

/**
  * @}
  */
#endif /* STM32F437xx || STM32F439xx || STM32F479xx */

#endif /* HAL_HASH_MODULE_ENABLED */
/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
