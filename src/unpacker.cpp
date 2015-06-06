// FireNode
// Copyright (c) 2013 Jon Evans
// http://craftyjon.com/projects/openlights/firenode
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "unpacker.h"
#include "color_correct.h"


Unpacker::Unpacker(int first, int last)
{
    first_strand = first;
    last_strand = last;
}


Unpacker::~Unpacker()
{
}


void Unpacker::assemble_data()
{
    QByteArray data;
    data.resize(strand_data[first_strand].length() * 8);
    data.fill(0);

    // LOL not optimal at all... I'm tired
    int pixelptr = 0;
    for (int dataptr = 0; dataptr < data.length(); dataptr += 8) {

        for (int i = first_strand; i < last_strand; i++) {

            uint8_t strand_shift = (i - first_strand);
            uint8_t strand_pixel_data = strand_data[i][pixelptr];

            data[dataptr + 0] = data[dataptr + 0] | ((strand_pixel_data & (1 << 7)) ? (1 << strand_shift) : 0);
            data[dataptr + 1] = data[dataptr + 1] | ((strand_pixel_data & (1 << 6)) ? (1 << strand_shift) : 0);
            data[dataptr + 2] = data[dataptr + 2] | ((strand_pixel_data & (1 << 5)) ? (1 << strand_shift) : 0);
            data[dataptr + 3] = data[dataptr + 3] | ((strand_pixel_data & (1 << 4)) ? (1 << strand_shift) : 0);
            data[dataptr + 4] = data[dataptr + 4] | ((strand_pixel_data & (1 << 3)) ? (1 << strand_shift) : 0);
            data[dataptr + 5] = data[dataptr + 5] | ((strand_pixel_data & (1 << 2)) ? (1 << strand_shift) : 0);
            data[dataptr + 6] = data[dataptr + 6] | ((strand_pixel_data & (1 << 1)) ? (1 << strand_shift) : 0);
            data[dataptr + 7] = data[dataptr + 7] | ((strand_pixel_data & (1 << 0)) ? (1 << strand_shift) : 0);
        }
        pixelptr++;
    }

    // Bytes 1 and 2 contained the length of the strand data.
    // Set them to zero because the Teensy interprets them as the delay time.
    // The rest of the data is interpreted as usual.
    data.prepend('\0');
    data.prepend('\0');

    // Start frame of video data
    data.prepend('*');

    emit data_ready(&data);
}


void Unpacker::unpack_data(QByteArray *data)
{   
    if (data->length() < 1) {
        return;
    }

    char cmd = data->at(0);

    if (cmd == 'B') {
        emit frame_begin();
        return;
    } else if (cmd == 'E') {
        emit frame_end();
        return;
    } else if (cmd == 'S') {

        // Process strand data
        Q_ASSERT(data->length() > 4);

        uint8_t strand = data->at(1);
        uint16_t len = (data->at(2) & 0xFF) | ((data->at(3) << 8) & 0xFF00);

        Q_ASSERT(strand < (MAX_STRANDS - 1));

        strand_data[strand] = data->right(len);

        // Swap color order
        for (int i = 0; i < len; i+=3) {
            char tmp = strand_data[strand][i];
            strand_data[strand][i] = strand_data[strand][i + 1];
            strand_data[strand][i + 1] = tmp;
        }

    }

#if 0

    do {
        new_data = data->left(4);
        data_len =  ((0x00FF & new_data.at(3)) << 8) + (0xFF & new_data.at(2));

        if ((unsigned char)new_data.at(1) == 0x10) {
            // Write BGR
            for (int j = 4; j < (4 + data_len); j += 3) {
                new_data += color_correct((*data)[j+2]);
                new_data += color_correct((*data)[j+1]);
                new_data += color_correct((*data)[j]);
            }
        } else if ((unsigned char)new_data.at(1) == 0x20) {
            // Write RGB
            new_data[1] = 0x10;
            for (int j = 4; j < (4 + data_len); j += 3) {
                new_data += color_correct(data->at(j));
                new_data += color_correct(data->at(j+1));
                new_data += color_correct(data->at(j+2));
            }
        }

        // Escape sequence
        escapes = 0;
        packet_size = new_data.size();
        for (int j = 2; j < packet_size; j++) {
            if (new_data.at(j) == (char)0x99) {
                new_data.insert(j + 1, (char)0x55);
                escapes++;
                packet_size++;
                j++;
            }
        }

        // Fixup strand numbering
        new_data[0] = new_data[0] +  1;

        //qDebug("data_len %d escapes %d", data_len, escapes);

        // Correct data length in case we added escape sequences
        new_data[2] = ((data_len + escapes) & 0xFF);
        new_data[3] = ((data_len + escapes) & 0xFF00) >> 8;

        // Start of frame sequence
        new_data.prepend((char)0x00);
        new_data.prepend((char)0x99);

        //qDebug() << "Strand data sending"; 

        emit data_ready(&new_data);
        
        data->remove(0, data_len + 4);

    } while (data->length() > 0);

#endif
}
