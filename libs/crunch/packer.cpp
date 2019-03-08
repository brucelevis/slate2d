/*
 
 MIT License
 
 Copyright (c) 2017 Chevy Ray Johnston
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 
 */

#include "packer.hpp"
#include "MaxRectsBinPack.h"
#include "GuillotineBinPack.h"
#include "binary.hpp"
#include <iostream>
#include <algorithm>

#include "console/console.h"
#include <cctype>

using namespace std;
using namespace rbp;

Packer::Packer(int width, int height, int pad)
: width(width), height(height), pad(pad)
{
    
}

void Packer::Pack(vector<Bitmap*>& bitmaps, bool verbose, bool unique, bool rotate)
{
    MaxRectsBinPack packer(width, height);
    
    int ww = 0;
    int hh = 0;
    while (!bitmaps.empty())
    {
        auto bitmap = bitmaps.back();
        
		if (verbose)
			Con_Printf("\t%i: %s\n", bitmaps.size(), bitmap->name.c_str());
        
        //Check to see if this is a duplicate of an already packed bitmap
        if (unique)
        {
            auto di = dupLookup.find(bitmap->hashValue);
            if (di != dupLookup.end() && bitmap->Equals(this->bitmaps[di->second]))
            {
                Point p = points[di->second];
                p.dupID = di->second;
                points.push_back(p);
                this->bitmaps.push_back(bitmap);
                bitmaps.pop_back();
                continue;
            }
        }
        
        //If it's not a duplicate, pack it into the atlas
        {
            Rect rect = packer.Insert(bitmap->width + pad, bitmap->height + pad, rotate, MaxRectsBinPack::RectBestShortSideFit);
            
            if (rect.width == 0 || rect.height == 0)
                break;
            
            if (unique)
                dupLookup[bitmap->hashValue] = static_cast<int>(points.size());
            
            //Check if we rotated it
            Point p;
            p.x = rect.x;
            p.y = rect.y;
            p.dupID = -1;
            p.rot = rotate && bitmap->width != (rect.width - pad);
            
            points.push_back(p);
            this->bitmaps.push_back(bitmap);
            bitmaps.pop_back();
            
            ww = max(rect.x + rect.width, ww);
            hh = max(rect.y + rect.height, hh);
        }
    }
    
    while (width / 2 >= ww)
        width /= 2;
    while( height / 2 >= hh)
        height /= 2;
}

void Packer::SavePng(const string& file)
{
    Bitmap bitmap(width, height);
    for (size_t i = 0, j = bitmaps.size(); i < j; ++i)
    {
        if (points[i].dupID < 0)
        {
            if (points[i].rot)
                bitmap.CopyPixelsRot(bitmaps[i], points[i].x, points[i].y);
            else
                bitmap.CopyPixels(bitmaps[i], points[i].x, points[i].y);
        }
    }
    bitmap.SaveAs(file);
}

void Packer::SaveBin(const string& name, ofstream& bin, bool trim, bool rotate)
{
    WriteString(bin, name);
    WriteShort(bin, (int16_t)bitmaps.size());
    for (size_t i = 0, j = bitmaps.size(); i < j; ++i)
    {
        WriteString(bin, bitmaps[i]->name);
        WriteShort(bin, (int16_t)points[i].x);
        WriteShort(bin, (int16_t)points[i].y);
        WriteShort(bin, (int16_t)bitmaps[i]->width);
        WriteShort(bin, (int16_t)bitmaps[i]->height);

        WriteShort(bin, (int16_t)bitmaps[i]->frameX);
        WriteShort(bin, (int16_t)bitmaps[i]->frameY);
        WriteShort(bin, (int16_t)bitmaps[i]->frameW);
        WriteShort(bin, (int16_t)bitmaps[i]->frameH);

        WriteByte(bin, points[i].rot ? 1 : 0);
    }
}