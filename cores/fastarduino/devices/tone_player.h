//   Copyright 2016-2019 Jean-Francois Poilpret
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.

/// @cond api

/**
 * @file
 * API to handle melody playing (tones as simple square waves) to a buzzer.
 */
#ifndef TONE_PLAYER_HH
#define TONE_PLAYER_HH

#include "../eeprom.h"
#include "../flash.h"
#include "tones.h"

//TODO write BaseTonePlayer (protected)
// write SyncTonePlayer (as before)
// write AsynTonePlayer (several flavours: events, ISR...?)
namespace devices::audio
{
	// Forward declaration
	template<board::Timer NTIMER, board::PWMPin OUTPUT> class AbstractTonePlayer;

	/**
	 * This namespace defines special "tones" which have an impact on how to play
	 * a melody. They are used by `TonePlayer` and based on special `Tone`s.
	 * @sa TonePlayer
	 * @sa Tone
	 */
	namespace SpecialTone
	{
		/**
		 * This special tone marks the end of a melody (as a sequence of `Tone`s).
		 */
		static constexpr const Tone END = Tone::USER0;

		/**
		 * This special tone marks the beginning of sequence that shall be repeated
		 * later.
		 * @sa REPEAT_END
		 */
		static constexpr const Tone REPEAT_START = Tone::USER1;

		/**
		 * This special tone marks the end of a repeating sequence (started with
		 * `REPEAT_START`). The associated duration in `TonePlay` has a special
		 * meaning: it indicates the number of times to repeat the sequence.
		 * @sa REPEAT_START
		 * @sa TonePlay::ms
		 */
		static constexpr const Tone REPEAT_END = Tone::USER2;
	}

	/**
	 * This struct is the unit data manipulated by `TonePlayer`: it describes one
	 * `Tone` along with its duration in milliseconds.
	 * It can also hold special tones as defined in `SpecialTone` namespace.
	 * 
	 * Note that, although it is quite easy to define melodies with `TonePlay`,
	 * it is less efficient in terms of code size and performance (this is due to
	 * the need to convert tone frequencies to Timer prescaler and counter). It
	 * is advised, to use `TonePlayer::QTonePlay` optimized structure instead.
	 * 
	 * @sa Tone
	 * @sa TonePlayer
	 * @sa SpecialTone
	 * @sa TonePlayer::QTonePlay
	 */
	class TonePlay
	{
	public:
		/**
		 * Default constructor, used only to declare an uninitialized
		 * `TonePlay` variable.
		 * You should always ensure you replace such a variable with one
		 * constructed with the next constructor.
		 * @sa TonePlay(Tone, uint16_t)
		 */
		TonePlay() = default;

		/**
		 * Construct a tone play with the provided tone and duration.
		 * 
		 * @param tone the `Tone` for this `TonePlay`; it will be automatically
		 * converted (at compile-time if @p t is a constant) to the proper
		 * timer prescaler and counter.
		 * @param ms the duration of this tone in milliseconds; this may have
		 * different meanings if @p t is a `SpecialTone`.
		 */
		constexpr TonePlay(Tone tone, uint16_t ms = 0) : tone_{tone}, ms_{ms} {}

	private:
		uint16_t duration() const
		{
			return ms_;
		}
		bool is_tone() const
		{
			return tone_ > Tone::SILENCE;
		}
		bool is_pause() const
		{
			return tone_ == Tone::SILENCE;
		}
		bool is_end() const
		{
			return tone_ == SpecialTone::END;
		}
		bool is_repeat_start() const
		{
			return tone_ == SpecialTone::REPEAT_START;
		}
		bool is_repeat_end() const
		{
			return tone_ == SpecialTone::REPEAT_END;
		}
		uint16_t repeat_count() const
		{
			return ms_;
		}

		template<board::Timer NTIMER, board::PWMPin OUTPUT>
		void generate_tone(ToneGenerator<NTIMER, OUTPUT>& generator) const
		{
			generator.start_tone(tone_);
		}

		Tone tone_;
		uint16_t ms_;

		template<board::Timer, board::PWMPin> friend class AbstractTonePlayer;
	};

	//TODO DOC?
	template<board::Timer NTIMER, board::PWMPin OUTPUT> class AbstractTonePlayer
	{
	private:
		static constexpr const uint16_t INTERTONE_DELAY_MS = 20;

	protected:
		using GENERATOR = ToneGenerator<NTIMER, OUTPUT>;

	public:
		/**
		 * An optimized surrogate to `TonePlay` structure.
		 * Contrarily to `TonePlay`, it does not store `Tone` (i.e. frequency) but
		 * Timer prescaler and counter for the desired frequency.
		 * The advantage is that when you construct a `QTonePlay` from constant
		 * tones, each tone wll be converted to timer prescaler and counter at 
		 * compile-time, hence the generated code is smaller and more efficient;
		 * this can be useful when your MCU is limited in data size.
		 */
		class QTonePlay
		{
			using CALC = timer::Calculator<NTIMER>;
			using TIMER = timer::Timer<NTIMER>;
			using PRESCALER = typename TIMER::PRESCALER;
			using COUNTER = typename TIMER::TYPE;

		public:
			/**
			 * Default constructor, used only to declare an uninitialized
			 * `QTonePlay` variable.
			 * You should always ensure you replace such a variable with one
			 * constructed with the next constructor.
			 * @sa QTonePlay(Tone, uint16_t)
			 */
			QTonePlay() = default;

			/**
			 * Construct an optimized tone play for the provided tone and duration.
			 * 
			 * @param tone the `Tone` for this `QTonePlay`; it will be automatically
			 * converted (at compile-time if @p t is a constant) to the proper
			 * timer prescaler and counter.
			 * @param ms the duration of this tone in milliseconds; this may have
			 * different meanings if @p t is a `SpecialTone`.
			 */
			constexpr QTonePlay(Tone tone, uint16_t ms = 0)
				: flags_{flags(tone)}, prescaler_{prescaler(tone)}, counter_{counter(tone)}, ms_{ms}
			{}

		private:
			uint16_t duration() const
			{
				return ms_;
			}
			bool is_tone() const
			{
				return flags_ == TONE;
			}
			bool is_pause() const
			{
				return flags_ == NONE;
			}
			bool is_end() const
			{
				return flags_ == END;
			}
			bool is_repeat_start() const
			{
				return flags_ == REPEAT_START;
			}
			bool is_repeat_end() const
			{
				return flags_ == REPEAT_END;
			}
			uint16_t repeat_count() const
			{
				return ms_;
			}

			void generate_tone(GENERATOR& generator) const
			{
				generator.start_tone(prescaler_, counter_);
			}

			static constexpr uint8_t TONE = 0x00;
			static constexpr uint8_t NONE = 0x01;
			static constexpr uint8_t END = 0x02;
			static constexpr uint8_t REPEAT_START = 0x04;
			static constexpr uint8_t REPEAT_END = 0x08;

			uint8_t flags_;
			PRESCALER prescaler_;
			COUNTER counter_;
			uint16_t ms_;

			static constexpr uint32_t period(Tone tone)
			{
				return ONE_SECOND / 2 / uint16_t(tone);
			}
			static constexpr PRESCALER prescaler(Tone tone)
			{
				return (tone > Tone::SILENCE ? CALC::CTC_prescaler(period(tone)) : PRESCALER::NO_PRESCALING);
			}
			static constexpr COUNTER counter(Tone tone)
			{
				return (tone > Tone::SILENCE ? CALC::CTC_counter(prescaler(tone), period(tone)) : 0);
			}
			static constexpr uint8_t flags(Tone tone)
			{
				if (tone == Tone::SILENCE) return NONE;
				if (tone == SpecialTone::END) return END;
				if (tone == SpecialTone::REPEAT_START) return REPEAT_START;
				if (tone == SpecialTone::REPEAT_END) return REPEAT_END;
				return TONE;
			}

			friend class AbstractTonePlayer<NTIMER, OUTPUT>;
		};

	protected:
		AbstractTonePlayer(GENERATOR& tone_generator) : generator_{tone_generator} {}

		void prepare(const TonePlay* melody)
		{
			prepare_(melody, load_sram<TonePlay>);
		}

		void prepare_eeprom(const TonePlay* melody)
		{
			prepare_(melody, load_eeprom<TonePlay>);
		}

		void prepare_flash(const TonePlay* melody)
		{
			prepare_(melody, load_flash<TonePlay>);
		}

		void prepare(const QTonePlay* melody)
		{
			prepare_(melody, load_sram<QTonePlay>);
		}

		void prepare_eeprom(const QTonePlay* melody)
		{
			prepare_(melody, load_eeprom<QTonePlay>);
		}

		void prepare_flash(const QTonePlay* melody)
		{
			prepare_(melody, load_flash<QTonePlay>);
		}

		uint16_t start_next_note()
		{
			if (is_qtone_)
				return start_next_(qtone_play_context_);
			else
				return start_next_(tone_play_context_);
		}

		uint16_t stop_current_note()
		{
			if (is_qtone_)
				return stop_current_(qtone_play_context_);
			else
				return stop_current_(tone_play_context_);
		}

		bool is_finished() const
		{
			return (!is_qtone_) && (tone_play_context_.loader == nullptr);
		}

	private:
		// Internal utility types
		template<typename TONEPLAY_>
		struct TonePlayContext
		{
			using TONEPLAY = TONEPLAY_;
			using LOAD_TONE = const TONEPLAY* (*) (const TONEPLAY* address, TONEPLAY& holder);

			TonePlayContext() = default;
			TonePlayContext(LOAD_TONE loader, const TONEPLAY* play)
			: loader{loader}, current_play{play}, repeat_play{}, repeat_times{}, no_delay{true} {}

			LOAD_TONE loader;
			const TONEPLAY* current_play;
			const TONEPLAY* repeat_play;
			int8_t repeat_times;
			bool no_delay;
		};

		using TONEPLAYCONTEXT = TonePlayContext<TonePlay>;
		using QTONEPLAYCONTEXT = TonePlayContext<QTonePlay>;

		void prepare_(const TonePlay* melody, typename TONEPLAYCONTEXT::LOAD_TONE load_tone)
		{
			is_qtone_ = false;
			tone_play_context_ = TONEPLAYCONTEXT{load_tone, melody};
		}

		void prepare_(const QTonePlay* melody, typename QTONEPLAYCONTEXT::LOAD_TONE load_tone)
		{
			is_qtone_ = true;
			qtone_play_context_ = QTONEPLAYCONTEXT{load_tone, melody};
		}

		template<typename CONTEXT>
		uint16_t start_next_(CONTEXT& context)
		{
			if (context.loader == nullptr) return 0;

			uint16_t delay = 0;
			typename CONTEXT::TONEPLAY holder;
			const typename CONTEXT::TONEPLAY* current = context.loader(context.current_play, holder);
			if (current->is_end())
			{
				context = CONTEXT{};
				is_qtone_ = false;
				return 0;
			}
			if (current->is_repeat_start())
			{
				context.repeat_play = context.current_play;
				context.repeat_times = -1;
				context.no_delay = true;
			}
			else if (current->is_repeat_end())
			{
				if (context.repeat_play)
				{
					if (context.repeat_times == -1) context.repeat_times = current->repeat_count();
					if (context.repeat_times--)
						context.current_play = context.repeat_play;
					else
						context.repeat_play = nullptr;
				}
				context.no_delay = true;
			}
			else
			{
				if (current->is_tone()) current->generate_tone(generator_);
				delay = current->duration();
				context.no_delay = false;
			}
			++context.current_play;
			return delay;
		}

		template<typename CONTEXT>
		uint16_t stop_current_(CONTEXT& context)
		{
			if (context.no_delay)
				return 0;
			generator_.stop_tone();
			return INTERTONE_DELAY_MS;
		}

		template<typename TONEPLAY>
		static const TONEPLAY* load_sram(const TONEPLAY* address, TONEPLAY& holder UNUSED)
		{
			return address;
		}
		template<typename TONEPLAY>
		static const TONEPLAY* load_eeprom(const TONEPLAY* address, TONEPLAY& holder)
		{
			eeprom::EEPROM::read(address, holder);
			return &holder;
		}
		template<typename TONEPLAY>
		static const TONEPLAY* load_flash(const TONEPLAY* address, TONEPLAY& holder)
		{
			flash::read_flash(address, holder);
			return &holder;
		}

		GENERATOR& generator_;
		bool is_qtone_;
		union
		{
			TONEPLAYCONTEXT tone_play_context_;
			QTONEPLAYCONTEXT qtone_play_context_;
		};
	};

	/**
	 * This API defines a player of melodies, defined as a sequence of tones and
	 * durations.
	 * 
	 * Melodies are defined as sequence of unit information, which can be either:
	 * - `TonePlay`s: easy to write in source code but not efficient in size of
	 * generated code
	 * - `TonePlayer::QTonePlay`s: requires more effort in source code, but 
	 * reduces generated code size
	 * Most methods exist in two flavours, one for each type.
	 * 
	 * With this API, played melodies can be stored on 3 possible locations:
	 * - in SRAM: this is useful when you get the melody from another support
	 * e.g. an external flash device
	 * - in Flash: this is the mostly used way as flash s the more abundant storage
	 * in AVR MCU
	 * - in EEPROM: this can be useful for short melodies, when you do not want
	 * to waste precious SRAM and Flash
	 * Each play API has 3 distinct methods, one for each storage strategy.
	 * 
	 * @tparam NTIMER the AVR timer to use for the underlying Timer
	 * @tparam OUTPUT the `board::PWMPin` connected to the buzzer;
	 * this must be the pin OCnA, where n is the AVR Timer number
	 * 
	 * @sa TonePlay
	 * @sa AbstractTonePlayer::QTonePlay
	 */
	template<board::Timer NTIMER, board::PWMPin OUTPUT> class TonePlayer : public AbstractTonePlayer<NTIMER, OUTPUT>
	{
		using BASE = AbstractTonePlayer<NTIMER, OUTPUT>;

	public:
		using QTonePlay = typename BASE::QTonePlay;
		using GENERATOR = typename BASE::GENERATOR;

		/**
		 * Create a new tone player, based on an existing `ToneGenerator`.
		 * @param tone_generator the `ToneGenerator` used to actually produce
		 * tones.
		 */
		explicit TonePlayer(GENERATOR& tone_generator) : BASE{tone_generator} {}

		/**
		 * Play a melody, defined by a sequence of `TonePlay`s, stored in SRAM.
		 * This method is blocking: it will return only when the melody is
		 * finished playing.
		 * @param melody a pointer, in SRAM, to the sequence of `TonePlay` to be
		 * played; the sequence MUST finish with a `SpecialTone::END`.
		 */
		void play(const TonePlay* melody)
		{
			this->prepare(melody);
			play_();
		}

		/**
		 * Play a melody, defined by a sequence of `TonePlay`s, stored in EEPROM.
		 * This method is blocking: it will return only when the melody is
		 * finished playing.
		 * @param melody a pointer, in EEPROM, to the sequence of `TonePlay` to 
		 * be played; the sequence MUST finish with a `SpecialTone::END`.
		 */
		void play_eeprom(const TonePlay* melody)
		{
			this->prepare_eeprom(melody);
			play_();
		}

		/**
		 * Play a melody, defined by a sequence of `TonePlay`s, stored in Flash.
		 * This method is blocking: it will return only when the melody is
		 * finished playing.
		 * @param melody a pointer, in Flash, to the sequence of `TonePlay` to 
		 * be played; the sequence MUST finish with a `SpecialTone::END`.
		 */
		void play_flash(const TonePlay* melody)
		{
			this->prepare_flash(melody);
			play_();
		}

		/**
		 * Play a melody, defined by a sequence of `QTonePlay`s, stored in SRAM.
		 * This method is blocking: it will return only when the melody is
		 * finished playing.
		 * @param melody a pointer, in SRAM, to the sequence of `QTonePlay` to be
		 * played; the sequence MUST finish with a `SpecialTone::END`.
		 */
		void play(const QTonePlay* melody)
		{
			this->prepare(melody);
			play_();
		}

		/**
		 * Play a melody, defined by a sequence of `QTonePlay`s, stored in EEPROM.
		 * This method is blocking: it will return only when the melody is
		 * finished playing.
		 * @param melody a pointer, in EEPROM, to the sequence of `QTonePlay` to be
		 * played; the sequence MUST finish with a `SpecialTone::END`.
		 */
		void play_eeprom(const QTonePlay* melody)
		{
			this->prepare_eeprom(melody);
			play_();
		}

		/**
		 * Play a melody, defined by a sequence of `QTonePlay`s, stored in Flash.
		 * This method is blocking: it will return only when the melody is
		 * finished playing.
		 * @param melody a pointer, in Flash, to the sequence of `QTonePlay` to be
		 * played; the sequence MUST finish with a `SpecialTone::END`.
		 */
		void play_flash(const QTonePlay* melody)
		{
			this->prepare_flash(melody);
			play_();
		}

		/**
		 * Stop playing current melody (if any).
		 * Playing is not immediate but will stop at the end of the current tone.
		 */
		void stop()
		{
			stop_ = true;
		}

	private:
		void play_()
		{
			stop_ = false;
			while (!stop_)
			{
				uint16_t delay = this->start_next_note();
				if (delay) time::delay_ms(delay);
				delay = this->stop_current_note();
				if (delay) time::delay_ms(delay);
				if (this->is_finished()) break;
			}
		}

		volatile bool stop_;
	};
}

#endif /* TONE_PLAYER_HH */
/// @endcond
