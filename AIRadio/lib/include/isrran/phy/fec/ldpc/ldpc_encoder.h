/**
 * Copyright 2013-2022 iSignal Research Labs Pvt Ltd.
 *
 * This file is part of isrRAN.
 *
 * isrRAN is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * isrRAN is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */

/*!
 * \file ldpc_encoder.h
 * \brief Declaration of the LDPC encoder.
 * \author David Gregoratti and Jesus Gomez
 * \date 2020
 *
 * \copyright iSignal Research Labs Pvt Ltd.
 *
 */

#ifndef ISRRAN_LDPCENCODER_H
#define ISRRAN_LDPCENCODER_H

#include "isrran/phy/fec/ldpc/base_graph.h"

/*!
 * \brief Types of LDPC encoder.
 */
typedef enum ISRRAN_API {
  ISRRAN_LDPC_ENCODER_C = 0, /*!< \brief Non-optimized encoder. */
#if LV_HAVE_AVX2
  ISRRAN_LDPC_ENCODER_AVX2, /*!< \brief SIMD-optimized encoder. */
#endif                      // LV_HAVE_AVX2
#if LV_HAVE_AVX512
  ISRRAN_LDPC_ENCODER_AVX512, /*!< \brief SIMD-optimized encoder. */
#endif                        // LV_HAVE_AVX512
} isrran_ldpc_encoder_type_t;

/*!
 * \brief Describes an LDPC encoder.
 */
typedef struct ISRRAN_API {
  void*              ptr;   /*!< \brief %Encoder auxiliary registers. */
  isrran_basegraph_t bg;    /*!< \brief Current base graph. */
  uint16_t           ls;    /*!< \brief Current lifting size. */
  uint8_t            bgN;   /*!< \brief Number of variable nodes in the BG. */
  uint16_t           liftN; /*!< \brief Number of variable nodes in the lifted graph. */
  uint8_t            bgM;   /*!< \brief Number of check nodes in the BG. */
  uint16_t           liftM; /*!< \brief Number of check nodes in the lifted graph. */
  uint8_t            bgK;   /*!< \brief Number of "uncoded bits" in the BG. */
  uint16_t           liftK; /*!< \brief Number of uncoded bits in the lifted graph. */
  uint16_t*          pcm;   /*!< \brief Pointer to the parity check matrix (compact form). */
  void (*free)(void*);      /*!< \brief Pointer to a "destructor". */
  /*! \brief Pointer to the encoder function. */
  int (*encode)(void*, const uint8_t*, uint8_t*, uint32_t, uint32_t);
  /*!  \brief Pointer to the encoder for the high-rate region. */
  void (*encode_high_rate)(void*, uint8_t*);
  /*!  \brief Pointer to the encoder for the high-rate region (SIMD-optimized version). */
  void (*encode_high_rate_avx2)(void*);
  /*!  \brief Pointer to the encoder for the high-rate region (SIMD-AVX512-optimized version). */
  void (*encode_high_rate_avx512)(void*);

} isrran_ldpc_encoder_t;

/*!
 * Initializes all the LDPC encoder variables according to the given base graph
 * and lifting size.
 * \param[out] q A pointer to a isrran_ldpc_encoder_t structure.
 * \param[in] type The encoder type.
 * \param[in] bg The desired base graph (BG1 or BG2).
 * \param[in] ls The desired lifting size.
 * \return An integer: 0 if the function executes correctly, -1 otherwise.
 */
ISRRAN_API int
isrran_ldpc_encoder_init(isrran_ldpc_encoder_t* q, isrran_ldpc_encoder_type_t type, isrran_basegraph_t bg, uint16_t ls);

/*!
 * The LDPC encoder "destructor": it frees all the resources allocated to the encoder.
 * \param[in] q A pointer to the dismantled encoder.
 */
ISRRAN_API void isrran_ldpc_encoder_free(isrran_ldpc_encoder_t* q);

/*!
 * Encodes a message into a codeword with the specified encoder.
 * \param[in] q A pointer to the desired encoder.
 * \param[in] input The message to encode.
 * \param[out] output The resulting codeword.
 * \param[in] input_length The number of uncoded bits in the input message.
 * \return An integer: 0 if the function executes correctly, -1 otherwise.
 */
ISRRAN_API int
isrran_ldpc_encoder_encode(isrran_ldpc_encoder_t* q, const uint8_t* input, uint8_t* output, uint32_t input_length);

/*!
 * Encodes a message into a codeword with the specified encoder.
 * \param[in] q A pointer to the desired encoder.
 * \param[in] input The message to encode.
 * \param[out] output The resulting codeword.
 * \param[in] input_length The number of uncoded bits in the input message.
 * \param[in] cdwd_rm_length The codeword length after rate matching.
 * \return An integer: 0 if the function executes correctly, -1 otherwise.
 */
ISRRAN_API int isrran_ldpc_encoder_encode_rm(isrran_ldpc_encoder_t* q,
                                             const uint8_t*         input,
                                             uint8_t*               output,
                                             uint32_t               input_length,
                                             uint32_t               cdwd_rm_length);

#endif // ISRRAN_LDPCENCODER_H
