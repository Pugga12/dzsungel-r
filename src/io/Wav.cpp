// Copyright (C) 2026  Adam Aptowitz
//
// This file is part of Dzsungel
//
// Dzsungel is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Dzsungel.  If not, see <http://www.gnu.org/license>
#include "io/Wav.hpp"

#define DR_WAV_IMPLEMENTATION
extern "C" {
#include "dr_wav.h"
}

namespace dzsungel::io {
    struct WAVWriter::Impl {
        drwav res;
    };

    bool WAVWriter::open(const std::string &filename, float sampleRate, uint32_t channels) {
        drwav_data_format format;
        format.container = drwav_container_riff;
        format.format = DR_WAVE_FORMAT_IEEE_FLOAT;
        format.channels = channels;
        format.sampleRate = static_cast<uint32_t>(sampleRate);
        format.bitsPerSample = 32;

        if (!drwav_init_file_write(&pImpl->res, filename.c_str(), &format, nullptr)) {
            return false;
        }
        open_ = true;
        return true;
    }

    size_t WAVWriter::write(SampleBuffer &sb) {
        if (open_) {
            return drwav_write_pcm_frames(&pImpl->res, sb.data.size(), sb.data.data());
        }
        return 0;
    }

    void WAVWriter::close() {
        drwav_uninit(&pImpl->res);
        open_ = false;
    }

    WAVWriter::~WAVWriter() {
        close();
    }

    WAVWriter::WAVWriter() : pImpl(std::make_unique<Impl>()) {}
} // namespace dzsungel::io
