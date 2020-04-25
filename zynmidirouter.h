/*
 * ******************************************************************
 * ZYNTHIAN PROJECT: ZynMidiRouter Library
 * 
 * MIDI router library: Implements the MIDI router & filter 
 * 
 * Copyright (C) 2015-2018 Fernando Moyano <jofemodo@zynthian.org>
 *
 * ******************************************************************
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the LICENSE.txt file.
 * 
 * ******************************************************************
 */

#include <jack/jack.h>
#include <jack/midiport.h>
#include <jack/ringbuffer.h>

//-----------------------------------------------------------------------------
// Library Initialization
//-----------------------------------------------------------------------------

int init_zynmidirouter();
int end_zynmidirouter();

//-----------------------------------------------------------------------------
// Data Structures
//-----------------------------------------------------------------------------

enum midi_event_type_enum {
	//Router-internal pseudo-message codes
	SWAP_EVENT=-3,
	IGNORE_EVENT=-2,
	THRU_EVENT=-1,
	NONE_EVENT=0,
	//Channel 3-bytes-messages
	NOTE_OFF=0x8,
	NOTE_ON=0x9,
	KEY_PRESS=0xA,
	CTRL_CHANGE=0xB,
	PITCH_BENDING=0xE,
	//Channel 2-bytes-messages
	PROG_CHANGE=0xC,
	CHAN_PRESS=0xD,
	//System 3-bytes-messages
	SONG_POSITION=0xF2,
	//System 2-bytes-messages
	TIME_CODE_QF=0xF1,
	SONG_SELECT=0xF3,
	//System 1-byte messages
	TUNE_REQUEST=0xF6,
	//System Real-Time
	TIME_CLOCK=0xF8,
	TRANSPORT_START=0xFA,
	TRANSPORT_CONTINUE=0xFB,
	TRANSPORT_STOP=0xFC,
	ACTIVE_SENSE=0xFE,
	MIDI_RESET=0xFF,
	//System Multi-byte (SysEx)
	SYSTEM_EXCLUSIVE=0xF0,
	END_SYSTEM_EXCLUSIVE=0xF7,
};

struct midi_event_st {
	enum midi_event_type_enum type;
	uint8_t chan;
	uint8_t num;
};

struct mf_arrow_st {
	uint8_t chan_from;
	uint8_t num_from;
	uint8_t chan_to;
	uint8_t num_to;
	enum midi_event_type_enum type;
};

struct mf_clone_st {
	int enabled;
	uint8_t cc[128];
};

static uint8_t default_cc_to_clone[]={ 1, 2, 64, 65, 66, 67, 68 };

struct midi_filter_st {
	int tuning_pitchbend;
	int master_chan;
	int active_chan;
	int last_active_chan;
	int auto_relmode;

	int transpose[16];
	struct mf_clone_st clone[16][16];
	struct midi_event_st event_map[8][16][128];

	uint8_t ctrl_mode[16][128];
	uint8_t ctrl_relmode_count[16][128];

	uint8_t last_ctrl_val[16][128];
	uint16_t last_pb_val[16];

	uint8_t note_state[16][128];
};
struct midi_filter_st midi_filter;

//-----------------------------------------------------------------------------
// MIDI Filter Functions
//-----------------------------------------------------------------------------

//MIDI filter initialization
int init_midi_router();
int end_midi_router();

//MIDI special featured channels
void set_midi_master_chan(int chan);
int get_midi_master_chan();
void set_midi_active_chan(int chan);
int get_midi_active_chan();

//MIDI filter fine tuning => Pitch-Bending based
void set_midi_filter_tuning_freq(int freq);
int get_midi_filter_tuning_pitchbend();

//MIDI filter transpose
void set_midi_filter_transpose(uint8_t chan, int offset);
int get_midi_filter_transpose(uint8_t chan);

//MIDI filter clone
void set_midi_filter_clone(uint8_t chan_from, uint8_t chan_to, int v);
int get_midi_filter_clone(uint8_t chan_from, uint8_t chan_to);
void reset_midi_filter_clone(uint8_t chan_from);
void set_midi_filter_clone_cc(uint8_t chan_from, uint8_t chan_to, uint8_t cc[128]);
uint8_t *get_midi_filter_clone_cc(uint8_t chan_from, uint8_t chan_to);
void reset_midi_filter_clone_cc(uint8_t chan_from, uint8_t chan_to);

//MIDI Filter Core functions
void set_midi_filter_event_map_st(struct midi_event_st *ev_from, struct midi_event_st *ev_to);
void set_midi_filter_event_map(enum midi_event_type_enum type_from, uint8_t chan_from, uint8_t num_from, enum midi_event_type_enum type_to, uint8_t chan_to, uint8_t num_to);
void set_midi_filter_event_ignore_st(struct midi_event_st *ev_from);
void set_midi_filter_event_ignore(enum midi_event_type_enum type_from, uint8_t chan_from, uint8_t num_from);
struct midi_event_st *get_midi_filter_event_map_st(struct midi_event_st *ev_from);
struct midi_event_st *get_midi_filter_event_map(enum midi_event_type_enum type_from, uint8_t chan_from, uint8_t num_from);
void del_midi_filter_event_map_st(struct midi_event_st *ev_filter);
void del_midi_filter_event_map(enum midi_event_type_enum type_from, uint8_t chan_from, uint8_t num_from);
void reset_midi_filter_event_map();

//MIDI Filter Mapping
void set_midi_filter_cc_map(uint8_t chan_from, uint8_t cc_from, uint8_t chan_to, uint8_t cc_to);
void set_midi_filter_cc_ignore(uint8_t chan, uint8_t cc_from);
uint8_t get_midi_filter_cc_map(uint8_t chan, uint8_t cc_from);
void del_midi_filter_cc_map(uint8_t chan, uint8_t cc_from);
void reset_midi_filter_cc_map();


//MIDI Learning Mode
int midi_learning_mode;
void set_midi_learning_mode(int mlm);

//MIDI Filter Swap Mapping
int get_mf_arrow_from(enum midi_event_type_enum type, uint8_t chan, uint8_t num, struct mf_arrow_st *arrow);
int get_mf_arrow_to(enum midi_event_type_enum type, uint8_t chan, uint8_t num, struct mf_arrow_st *arrow);
int set_midi_filter_cc_swap(uint8_t chan_from, uint8_t num_from, uint8_t chan_to, uint8_t num_to);
int del_midi_filter_cc_swap(uint8_t chan, uint8_t num);
uint8_t get_midi_filter_cc_swap(uint8_t chan, uint8_t num);

//-----------------------------------------------------------------------------
// Zynmidi Ports
//-----------------------------------------------------------------------------

#define JACK_MIDI_BUFFER_SIZE 4096

#define FLAG_ZMOP_TUNING 64

#define FLAG_ZMIP_UI 1
#define FLAG_ZMIP_ZYNCODER 2
#define FLAG_ZMIP_CLONE 4
#define FLAG_ZMIP_FILTER 8
#define FLAG_ZMIP_SWAP 16
#define FLAG_ZMIP_TRANSPOSE 32
#define FLAG_ZMIP_TUNING 64

#define ZMOP_MAIN 0
#define ZMOP_MIDI 1
#define ZMOP_NET 2
#define ZMOP_CH0 3
#define ZMOP_CH1 4
#define ZMOP_CH2 5
#define ZMOP_CH3 6
#define ZMOP_CH4 7
#define ZMOP_CH5 8
#define ZMOP_CH6 8
#define ZMOP_CH7 10
#define ZMOP_CH8 11
#define ZMOP_CH9 12
#define ZMOP_CH10 13
#define ZMOP_CH11 14
#define ZMOP_CH12 15
#define ZMOP_CH13 16
#define ZMOP_CH14 17
#define ZMOP_CH15 18
#define ZMOP_STEP 19
#define ZMOP_CTRL 20
#define MAX_NUM_ZMOPS 21

#define ZMIP_MAIN 0
#define ZMIP_NET 1
#define ZMIP_SEQ 2
#define ZMIP_CTRL 3
#define ZMIP_STEP 4
#define MAX_NUM_ZMIPS 5

#define ZMOP_MAIN_FLAGS (FLAG_ZMOP_TUNING)

#define ZMIP_MAIN_FLAGS (FLAG_ZMIP_UI|FLAG_ZMIP_ZYNCODER|FLAG_ZMIP_CLONE|FLAG_ZMIP_FILTER|FLAG_ZMIP_SWAP|FLAG_ZMIP_TRANSPOSE|FLAG_ZMIP_TUNING)
#define ZMIP_SEQ_FLAGS (FLAG_ZMIP_UI|FLAG_ZMIP_ZYNCODER)
#define ZMIP_CTRL_FLAGS (FLAG_ZMIP_UI)

struct zmop_st {
	jack_port_t *jport;
	uint8_t data[JACK_MIDI_BUFFER_SIZE];
	int n_data;
	int midi_channel;
	int n_connections;
	uint32_t flags;
};
struct zmop_st zmops[MAX_NUM_ZMOPS];

int zmop_init(int iz, char *name, int ch, uint32_t flags);
int zmop_push_data(int iz, jack_midi_event_t ev, int ch);
int zmop_clear_data(int iz);
int zmops_clear_data();
int zmop_set_flags(int iz, uint32_t flags);
int zoip_has_flag(int iz, uint32_t flag);

struct zmip_st {
	jack_port_t *jport;
	int fwd_zmops[MAX_NUM_ZMOPS];
	uint32_t flags;
};
struct zmip_st zmips[MAX_NUM_ZMIPS];

int zmip_init(int iz, char *name, uint32_t flags);
int zmip_set_forward(int izmip, int izmop, int fwd);
int zmip_set_flags(int iz, uint32_t flags);
int zmip_has_flag(int iz, uint32_t flag);

//-----------------------------------------------------------------------------
// Jack MIDI Process
//-----------------------------------------------------------------------------

jack_client_t *jack_client;

int init_jack_midi(char *name);
int end_jack_midi();
int jack_process(jack_nframes_t nframes, void *arg);

//-----------------------------------------------------------------------------
// MIDI Input Events Buffer Management and Send functions
//-----------------------------------------------------------------------------

#define ZYNMIDI_BUFFER_SIZE 1024

//-----------------------------------------------------
// MIDI Internal Input <= UI and internal
//-----------------------------------------------------

jack_ringbuffer_t *jack_ring_output_buffer;
int write_internal_midi_event(uint8_t *event, int event_size);

int zynmidi_send_note_off(uint8_t chan, uint8_t note, uint8_t vel);
int zynmidi_send_note_on(uint8_t chan, uint8_t note, uint8_t vel);
int zynmidi_send_ccontrol_change(uint8_t chan, uint8_t ctrl, uint8_t val);
int zynmidi_send_program_change(uint8_t chan, uint8_t prgm);
int zynmidi_send_pitchbend_change(uint8_t chan, uint16_t pb);
int zynmidi_send_master_ccontrol_change(uint8_t ctrl, uint8_t val);
int zynmidi_send_all_notes_off();
int zynmidi_send_all_notes_off_chan(uint8_t chan);

//-----------------------------------------------------
// MIDI Controller Feedback <= UI and internal
//-----------------------------------------------------

jack_ringbuffer_t *jack_ring_ctrlfb_buffer;
int write_ctrlfb_midi_event(uint8_t *event, int event_size);

int ctrlfb_send_note_off(uint8_t chan, uint8_t note, uint8_t vel);
int ctrlfb_send_note_on(uint8_t chan, uint8_t note, uint8_t vel);
int ctrlfb_send_ccontrol_change(uint8_t chan, uint8_t ctrl, uint8_t val);
int ctrlfb_send_program_change(uint8_t chan, uint8_t prgm);
int ctrlfb_send_pitchbend_change(uint8_t chan, uint16_t pb);

//-----------------------------------------------------------------------------
// MIDI Internal Ouput Events Buffer => UI
//-----------------------------------------------------------------------------

int init_zynmidi_buffer();
int write_zynmidi(uint32_t ev);
uint32_t read_zynmidi();

int write_zynmidi_ccontrol_change(uint8_t chan, uint8_t num, uint8_t val);
int write_zynmidi_note_on(uint8_t chan, uint8_t num, uint8_t val);
int write_zynmidi_note_off(uint8_t chan, uint8_t num, uint8_t val);
int write_zynmidi_program_change(uint8_t chan, uint8_t num);

//-----------------------------------------------------------------------------
// MIDI Controller Auto-Mode (Absolut <=> Relative)
//-----------------------------------------------------------------------------

int midi_ctrl_automode;
void set_midi_ctrl_automode(int mcam);


//-----------------------------------------------------------------------------
