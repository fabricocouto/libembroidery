static char xxxDecodeByte(unsigned char inputByte)
{
    if (inputByte >= 0x80)
        return (char)((-~inputByte) - 1);
    return ((char)inputByte);
}

/*! Reads a file with the given \a fileName and loads the data into \a pattern.
 *  Returns \c true if successful, otherwise returns \c false. */
static int readXxx(EmbPattern* pattern, EmbFile* file, const char* fileName)
{
    unsigned char b0, b1;
    int dx = 0, dy = 0;
    int flags, numberOfColors, paletteOffset, i;
    char endOfStream = 0;
    char thisStitchJump = 0;
    EmbThread thread;
    EmbStitch st;

    embFile_seek(file, 0x27, SEEK_SET);
    numberOfColors = binaryReadInt16(file);
    embFile_seek(file, 0xFC, SEEK_SET);
    paletteOffset = binaryReadInt32(file);
    embFile_seek(file, paletteOffset + 6, SEEK_SET);

    for (i = 0; i < numberOfColors; i++) {
        binaryReadByte(file);
        thread.color.r = binaryReadByte(file);
        thread.color.g = binaryReadByte(file);
        thread.color.b = binaryReadByte(file);
        embPattern_addThread(pattern, thread);
    }
    embFile_seek(file, 0x100, SEEK_SET);

    for (i = 0; !endOfStream && embFile_tell(file) < paletteOffset; i++) {
        flags = NORMAL;
        if (thisStitchJump)
            flags = TRIM;
        thisStitchJump = 0;
        b0 = binaryReadByte(file);
        b1 = binaryReadByte(file);

        if (b0 == 0x7E || b0 == 0x7D) /* TODO: ARE THERE OTHER BIG JUMP CODES? */
        {
            dx = b1 + (binaryReadByte(file) << 8);
            dx = ((short)dx);
            dy = binaryReadInt16(file);
            flags = TRIM;
        } else if (b0 == 0x7F) {
            if (b1 != 0x17 && b1 != 0x46 && b1 >= 8) /* TODO: LOOKS LIKE THESE CODES ARE IN THE HEADER */
            {
                b0 = 0;
                b1 = 0;
                thisStitchJump = 1;
                flags = STOP;
            } else if (b1 == 1) {
                flags = TRIM;
                b0 = binaryReadByte(file);
                b1 = binaryReadByte(file);
            } else {
                continue;
            }
            dx = xxxDecodeByte(b0);
            dy = xxxDecodeByte(b1);
        } else {
            dx = xxxDecodeByte(b0);
            dy = xxxDecodeByte(b1);
        }
        embPattern_addStitchRel(pattern, dx / 10.0, dy / 10.0, flags, 1);
    }
    /*
    lastStitch = pattern->stitchList;
    secondLast = 0;
    if (lastStitch)
 {
        while (lastStitch->next)
        {
            secondLast = lastStitch;
            lastStitch = lastStitch->next;
        }
        if((!pattern->stitchList) && lastStitch->stitch.flags == STOP && secondLast)
        {
            free(lastStitch);
            lastStitch = 0;
            secondLast->next = NULL;
            embPattern_changeColor(pattern, pattern->currentColorIndex - 1);
        }
    }
    
    Is this trimming the last stitch... and?
    */

    return 1;
}

static void xxxEncodeStop(EmbFile* file, EmbStitch s)
{
    binaryWriteByte(file, (unsigned char)0x7F);
    binaryWriteByte(file, (unsigned char)(s.color + 8));
}

static void xxxEncodeStitch(EmbFile* file, double deltaX, double deltaY, int flags)
{
    if ((flags & (JUMP | TRIM)) && (fabs(deltaX) > 124 || fabs(deltaY) > 124)) {
        binaryWriteByte(file, 0x7E);
        binaryWriteShort(file, (short)deltaX);
        binaryWriteShort(file, (short)deltaY);
    } else {
        /* TODO: Verify this works after changing this to unsigned char */
        binaryWriteByte(file, (unsigned char)roundDouble(deltaX));
        binaryWriteByte(file, (unsigned char)roundDouble(deltaY));
    }
}

static void xxxEncodeDesign(EmbFile* file, EmbPattern* p)
{
    double thisX = 0.0f;
    double thisY = 0.0f;
    double previousX, previousY, deltaX, deltaY;
    EmbStitch s;
    int i;

    if (p->stitchList) {
        thisX = (float)p->stitchList->stitch[0].x;
        thisY = (float)p->stitchList->stitch[0].y;
    } else {
        return;
    }

    for (i = 0; i < p->stitchList->count; i++) {
        s = p->stitchList->stitch[i];
        previousX = thisX;
        previousY = thisY;
        thisX = s.x;
        thisY = s.y;
        deltaX = thisX - previousX;
        deltaY = thisY - previousY;
        if (s.flags & STOP) {
            xxxEncodeStop(file, s);
        } else if (s.flags & END) {
        } else {
            xxxEncodeStitch(file, deltaX * 10.0f, deltaY * 10.0f, s.flags);
        }
    }
}

/*! Writes the data from \a pattern to a file with the given \a fileName.
 *  Returns \c true if successful, otherwise returns \c false. */
static int writeXxx(EmbPattern* pattern, EmbFile* file, const char* fileName)
{
    int i;
    EmbRect rect;
    EmbColor c;
    int endOfStitches;

    embPattern_correctForMaxStitchLength(pattern, 124, 127);

    for (i = 0; i < 0x17; i++) {
        binaryWriteByte(file, 0x00);
    }
    binaryWriteUInt(file, (unsigned int)pattern->stitchList->count);
    for (i = 0; i < 0x0C; i++) {
        binaryWriteByte(file, 0x00);
    }
    binaryWriteUShort(file, (unsigned short)pattern->threads->count);
    binaryWriteShort(file, 0x0000);

    rect = embPattern_calcBoundingBox(pattern);
    binaryWriteShort(file, (short)(embRect_width(rect) * 10.0));
    binaryWriteShort(file, (short)(embRect_height(rect) * 10.0));

    binaryWriteShort(file, (short)(embRect_width(rect) / 2.0 * 10)); /*TODO: xEnd from start point x=0 */
    binaryWriteShort(file, (short)(embRect_height(rect) / 2.0 * 10)); /*TODO: yEnd from start point y=0 */
    binaryWriteShort(file, (short)(embRect_width(rect) / 2.0 * 10)); /*TODO: left from start x = 0     */
    binaryWriteShort(file, (short)(embRect_height(rect) / 2.0 * 10)); /*TODO: bottom from start y = 0   */
    for (i = 0; i < 0xC5; i++) {
        binaryWriteByte(file, 0x00);
    }
    binaryWriteInt(file, 0x0000); /* place holder for end of stitches */

    xxxEncodeDesign(file, pattern);

    endOfStitches = embFile_tell(file);

    embFile_seek(file, 0xFC, SEEK_SET);

    binaryWriteUInt(file, endOfStitches);

    embFile_seek(file, 0, SEEK_END);

    binaryWriteByte(file, 0x7F); /* is this really correct? */
    binaryWriteByte(file, 0x7F);
    binaryWriteByte(file, 0x03);
    binaryWriteByte(file, 0x14);
    binaryWriteByte(file, 0x00);
    binaryWriteByte(file, 0x00);

    for (i = 0; i < 22; i++) {
        if (i < pattern->threads->count) {
            c = pattern->threads->thread[i].color;
            binaryWriteByte(file, 0x00);
            binaryWriteByte(file, c.r);
            binaryWriteByte(file, c.g);
            binaryWriteByte(file, c.b);
        } else {
            binaryWriteUInt(file, 0x01000000);
        }
    }

    binaryWriteByte(file, 0x00);
    binaryWriteByte(file, 0x01);
    return 1;
}

