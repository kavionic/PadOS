// This file is part of PadOS.
//
// Copyright (C) 2017-2018 Kurt Skauen <http://kavionic.com/>
//
// PadOS is free software : you can redistribute it and / or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// PadOS is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with PadOS. If not, see <http://www.gnu.org/licenses/>.
///////////////////////////////////////////////////////////////////////////////
// Created: 24.11.2017 21:41:16

#pragma once


#define TC_CMR_CPCSTOP_Pos                  6
#define TC_CMR_CPCSTOP_Msk                  (0x1U << TC_CMR_CPCSTOP_Pos)

#define TC_CMR_CPCDIS_Pos                   7
#define TC_CMR_CPCDIS_Msk                   (0x1U << TC_CMR_CPCDIS_Pos)

#define TC_CMR_EEVTEDG_Pos                  8
#define TC_CMR_EEVTEDG_Msk                  (0x3U << TC_CMR_EEVTEDG_Pos)
#define TC_CMR_EEVTEDG(value)               (TC_CMR_EEVTEDG_Msk & ((value) << TC_CMR_EEVTEDG_Pos))
#define   TC_CMR_EEVTEDG_NONE_Val           (0x0U)
#define   TC_CMR_EEVTEDG_RISING_Val         (0x1U)
#define   TC_CMR_EEVTEDG_FALLING_Val        (0x2U)
#define   TC_CMR_EEVTEDG_EDGE_Val           (0x3U)
#define TC_CMR_EEVTEDG_NONE                 (TC_CMR_EEVTEDG_NONE_Val << TC_CMR_EEVTEDG_Pos)
#define TC_CMR_EEVTEDG_RISING               (TC_CMR_EEVTEDG_RISING_Val << TC_CMR_EEVTEDG_Pos)
#define TC_CMR_EEVTEDG_FALLING              (TC_CMR_EEVTEDG_FALLING_Val << TC_CMR_EEVTEDG_Pos)
#define TC_CMR_EEVTEDG_EDGE                 (TC_CMR_EEVTEDG_EDGE_Val << TC_CMR_EEVTEDG_Pos)

#define TC_CMR_EEVT_Pos                     10
#define TC_CMR_EEVT_Msk                     (0x3U << TC_CMR_EEVT_Pos)
#define TC_CMR_EEVT(value)                  (TC_CMR_EEVT_Msk & ((value) << TC_CMR_EEVT_Pos))
#define   TC_CMR_EEVT_TIOB_Val              (0x0U)
#define   TC_CMR_EEVT_XC0_Val               (0x1U)
#define   TC_CMR_EEVT_XC1_Val               (0x2U)
#define   TC_CMR_EEVT_XC2_Val               (0x3U)
#define TC_CMR_EEVT_TIOB                    (TC_CMR_EEVT_TIOB_Val << TC_CMR_EEVT_Pos)
#define TC_CMR_EEVT_XC0                     (TC_CMR_EEVT_XC0_Val << TC_CMR_EEVT_Pos)
#define TC_CMR_EEVT_XC1                     (TC_CMR_EEVT_XC1_Val << TC_CMR_EEVT_Pos)
#define TC_CMR_EEVT_XC2                     (TC_CMR_EEVT_XC2_Val << TC_CMR_EEVT_Pos)

#define TC_CMR_ENETRG_Pos                   12
#define TC_CMR_ENETRG_Msk                   (0x1U << TC_CMR_ENETRG_Pos)

#define TC_CMR_WAVESEL_Pos                  13
#define TC_CMR_WAVESEL_Msk                  (0x3U << TC_CMR_WAVESEL_Pos)
#define TC_CMR_WAVESEL(value)               (TC_CMR_WAVESEL_Msk & ((value) << TC_CMR_WAVESEL_Pos))
#define   TC_CMR_WAVESEL_UP_Val             (0x0U)
#define   TC_CMR_WAVESEL_UPDOWN_Val         (0x1U)
#define   TC_CMR_WAVESEL_UP_RC_Val          (0x2U)
#define   TC_CMR_WAVESEL_UPDOWN_RC_Val      (0x3U)
#define TC_CMR_WAVESEL_UP                   (TC_CMR_WAVESEL_UP_Val << TC_CMR_WAVESEL_Pos)
#define TC_CMR_WAVESEL_UPDOWN               (TC_CMR_WAVESEL_UPDOWN_Val << TC_CMR_WAVESEL_Pos)
#define TC_CMR_WAVESEL_UP_RC                (TC_CMR_WAVESEL_UP_RC_Val << TC_CMR_WAVESEL_Pos)
#define TC_CMR_WAVESEL_UPDOWN_RC            (TC_CMR_WAVESEL_UPDOWN_RC_Val << TC_CMR_WAVESEL_Pos)

#define TC_CMR_ACPA_Pos                     16
#define TC_CMR_ACPA_Msk                     (0x3U << TC_CMR_ACPA_Pos)
#define TC_CMR_ACPA(value)                  (TC_CMR_ACPA_Msk & ((value) << TC_CMR_ACPA_Pos))
#define   TC_CMR_ACPA_NONE_Val              (0x0U)
#define   TC_CMR_ACPA_SET_Val               (0x1U)
#define   TC_CMR_ACPA_CLEAR_Val             (0x2U)
#define   TC_CMR_ACPA_TOGGLE_Val            (0x3U)
#define TC_CMR_ACPA_NONE                    (TC_CMR_ACPA_NONE_Val << TC_CMR_ACPA_Pos)
#define TC_CMR_ACPA_SET                     (TC_CMR_ACPA_SET_Val << TC_CMR_ACPA_Pos)
#define TC_CMR_ACPA_CLEAR                   (TC_CMR_ACPA_CLEAR_Val << TC_CMR_ACPA_Pos)
#define TC_CMR_ACPA_TOGGLE                  (TC_CMR_ACPA_TOGGLE_Val << TC_CMR_ACPA_Pos)

#define TC_CMR_ACPC_Pos                     18
#define TC_CMR_ACPC_Msk                     (0x3U << TC_CMR_ACPC_Pos)
#define TC_CMR_ACPC(value)                  (TC_CMR_ACPC_Msk & ((value) << TC_CMR_ACPC_Pos))
#define   TC_CMR_ACPC_NONE_Val              (0x0U)
#define   TC_CMR_ACPC_SET_Val               (0x1U)
#define   TC_CMR_ACPC_CLEAR_Val             (0x2U)
#define   TC_CMR_ACPC_TOGGLE_Val            (0x3U)
#define TC_CMR_ACPC_NONE                    (TC_CMR_ACPC_NONE_Val << TC_CMR_ACPC_Pos)
#define TC_CMR_ACPC_SET                     (TC_CMR_ACPC_SET_Val << TC_CMR_ACPC_Pos)
#define TC_CMR_ACPC_CLEAR                   (TC_CMR_ACPC_CLEAR_Val << TC_CMR_ACPC_Pos)
#define TC_CMR_ACPC_TOGGLE                  (TC_CMR_ACPC_TOGGLE_Val << TC_CMR_ACPC_Pos)

#define TC_CMR_AEEVT_Pos                    20
#define TC_CMR_AEEVT_Msk                    (0x3U << TC_CMR_AEEVT_Pos)
#define TC_CMR_AEEVT(value)                 (TC_CMR_AEEVT_Msk & ((value) << TC_CMR_AEEVT_Pos))
#define   TC_CMR_AEEVT_NONE_Val             (0x0U)
#define   TC_CMR_AEEVT_SET_Val              (0x1U)
#define   TC_CMR_AEEVT_CLEAR_Val            (0x2U)
#define   TC_CMR_AEEVT_TOGGLE_Val           (0x3U)
#define TC_CMR_AEEVT_NONE                   (TC_CMR_AEEVT_NONE_Val << TC_CMR_AEEVT_Pos)
#define TC_CMR_AEEVT_SET                    (TC_CMR_AEEVT_SET_Val << TC_CMR_AEEVT_Pos)
#define TC_CMR_AEEVT_CLEAR                  (TC_CMR_AEEVT_CLEAR_Val << TC_CMR_AEEVT_Pos)
#define TC_CMR_AEEVT_TOGGLE                 (TC_CMR_AEEVT_TOGGLE_Val << TC_CMR_AEEVT_Pos)

#define TC_CMR_ASWTRG_Pos                   22
#define TC_CMR_ASWTRG_Msk                   (0x3U << TC_CMR_ASWTRG_Pos)
#define TC_CMR_ASWTRG(value)                (TC_CMR_ASWTRG_Msk & ((value) << TC_CMR_ASWTRG_Pos))
#define   TC_CMR_ASWTRG_NONE_Val            (0x0U)
#define   TC_CMR_ASWTRG_SET_Val             (0x1U)
#define   TC_CMR_ASWTRG_CLEAR_Val           (0x2U)
#define   TC_CMR_ASWTRG_TOGGLE_Val          (0x3U)
#define TC_CMR_ASWTRG_NONE                  (TC_CMR_ASWTRG_NONE_Val << TC_CMR_ASWTRG_Pos)
#define TC_CMR_ASWTRG_SET                   (TC_CMR_ASWTRG_SET_Val << TC_CMR_ASWTRG_Pos)
#define TC_CMR_ASWTRG_CLEAR                 (TC_CMR_ASWTRG_CLEAR_Val << TC_CMR_ASWTRG_Pos)
#define TC_CMR_ASWTRG_TOGGLE                (TC_CMR_ASWTRG_TOGGLE_Val << TC_CMR_ASWTRG_Pos)

#define TC_CMR_BCPB_Pos                     24
#define TC_CMR_BCPB_Msk                     (0x3U << TC_CMR_BCPB_Pos)
#define TC_CMR_BCPB(value)                  (TC_CMR_BCPB_Msk & ((value) << TC_CMR_BCPB_Pos))
#define   TC_CMR_BCPB_NONE_Val              (0x0U)
#define   TC_CMR_BCPB_SET_Val               (0x1U)
#define   TC_CMR_BCPB_CLEAR_Val             (0x2U)
#define   TC_CMR_BCPB_TOGGLE_Val            (0x3U)
#define TC_CMR_BCPB_NONE                    (TC_CMR_BCPB_NONE_Val << TC_CMR_BCPB_Pos)
#define TC_CMR_BCPB_SET                     (TC_CMR_BCPB_SET_Val << TC_CMR_BCPB_Pos)
#define TC_CMR_BCPB_CLEAR                   (TC_CMR_BCPB_CLEAR_Val << TC_CMR_BCPB_Pos)
#define TC_CMR_BCPB_TOGGLE                  (TC_CMR_BCPB_TOGGLE_Val << TC_CMR_BCPB_Pos)

#define TC_CMR_BCPC_Pos                     26
#define TC_CMR_BCPC_Msk                     (0x3U << TC_CMR_BCPC_Pos)
#define TC_CMR_BCPC(value)                  (TC_CMR_BCPC_Msk & ((value) << TC_CMR_BCPC_Pos))
#define   TC_CMR_BCPC_NONE_Val              (0x0U)
#define   TC_CMR_BCPC_SET_Val               (0x1U)
#define   TC_CMR_BCPC_CLEAR_Val             (0x2U)
#define   TC_CMR_BCPC_TOGGLE_Val            (0x3U)
#define TC_CMR_BCPC_NONE                    (TC_CMR_BCPC_NONE_Val << TC_CMR_BCPC_Pos)
#define TC_CMR_BCPC_SET                     (TC_CMR_BCPC_SET_Val << TC_CMR_BCPC_Pos)
#define TC_CMR_BCPC_CLEAR                   (TC_CMR_BCPC_CLEAR_Val << TC_CMR_BCPC_Pos)
#define TC_CMR_BCPC_TOGGLE                  (TC_CMR_BCPC_TOGGLE_Val << TC_CMR_BCPC_Pos)

#define TC_CMR_BEEVT_Pos                    28
#define TC_CMR_BEEVT_Msk                    (0x3U << TC_CMR_BEEVT_Pos)
#define TC_CMR_BEEVT(value)                 (TC_CMR_BEEVT_Msk & ((value) << TC_CMR_BEEVT_Pos))
#define   TC_CMR_BEEVT_NONE_Val             (0x0U)
#define   TC_CMR_BEEVT_SET_Val              (0x1U)
#define   TC_CMR_BEEVT_CLEAR_Val            (0x2U)
#define   TC_CMR_BEEVT_TOGGLE_Val           (0x3U)
#define TC_CMR_BEEVT_NONE                   (TC_CMR_BEEVT_NONE_Val << TC_CMR_BEEVT_Pos)
#define TC_CMR_BEEVT_SET                    (TC_CMR_BEEVT_SET_Val << TC_CMR_BEEVT_Pos)
#define TC_CMR_BEEVT_CLEAR                  (TC_CMR_BEEVT_CLEAR_Val << TC_CMR_BEEVT_Pos)
#define TC_CMR_BEEVT_TOGGLE                 (TC_CMR_BEEVT_TOGGLE_Val << TC_CMR_BEEVT_Pos)

#define TC_CMR_BSWTRG_Pos                   30
#define TC_CMR_BSWTRG_Msk                   (0x3U << TC_CMR_BSWTRG_Pos)
#define TC_CMR_BSWTRG(value)                (TC_CMR_BSWTRG_Msk & ((value) << TC_CMR_BSWTRG_Pos))
#define   TC_CMR_BSWTRG_NONE_Val            (0x0U)
#define   TC_CMR_BSWTRG_SET_Val             (0x1U)
#define   TC_CMR_BSWTRG_CLEAR_Val           (0x2U)
#define   TC_CMR_BSWTRG_TOGGLE_Val          (0x3U)
#define TC_CMR_BSWTRG_NONE                  (TC_CMR_BSWTRG_NONE_Val << TC_CMR_BSWTRG_Pos)
#define TC_CMR_BSWTRG_SET                   (TC_CMR_BSWTRG_SET_Val << TC_CMR_BSWTRG_Pos)
#define TC_CMR_BSWTRG_CLEAR                 (TC_CMR_BSWTRG_CLEAR_Val << TC_CMR_BSWTRG_Pos)
#define TC_CMR_BSWTRG_TOGGLE                (TC_CMR_BSWTRG_TOGGLE_Val << TC_CMR_BSWTRG_Pos)
