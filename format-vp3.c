static unsigned char* vp3ReadString(EmbFile* file)
{
    int stringLength = 0;
    unsigned char* charString = 0;
    if (!file) {
        embLog("ERROR: format-vp3.c vp3ReadString(), file argument is null\n");
        return 0;
    }
    stringLength = binaryReadInt16BE(file);
    charString = (unsigned char*)malloc(stringLength);
    if (!charString) {
        embLog("ERROR: format-vp3.c vp3ReadString(), cannot allocate memory for charString\n");
        return 0;
    }
    binaryReadBytes(file, charString, stringLength); /* TODO: check return value */
    return charString;
}

static int vp3Decode(unsigned char inputByte)
{
    if (inputByte > 0x80) {
        return (int)-((unsigned char)((~inputByte) + 1));
    }
    return ((int)inputByte);
}

static short vp3DecodeInt16(unsigned short inputByte)
{
    if (inputByte > 0x8000) {
        return -((short)((~inputByte) + 1));
    }
    return ((short)inputByte);
}

typedef struct _vp3Hoop {
    int right;
    int bottom;
    int left;
    int top;
    int threadLength;
    char unknown2;
    unsigned char numberOfColors;
    unsigned short unknown3;
    int unknown4;
    int numberOfBytesRemaining;

    int xOffset;
    int yOffset;

    unsigned char byte1;
    unsigned char byte2;
    unsigned char byte3;

    /* Centered hoop dimensions */
    int right2;
    int left2;
    int bottom2;
    int top2;

    int width;
    int height;
} vp3Hoop;

static vp3Hoop vp3ReadHoopSection(EmbFile* file)
{
    vp3Hoop hoop;

    if (!file) {
        embLog("ERROR: format-vp3.c vp3ReadHoopSection(), file argument is null\n");
        hoop.bottom = 0;
        hoop.left = 0;
        hoop.right = 0;
        hoop.top = 0;
        return hoop;
    }

    hoop.right = binaryReadInt32BE(file);
    hoop.bottom = binaryReadInt32BE(file);
    hoop.left = binaryReadInt32BE(file);
    hoop.top = binaryReadInt32BE(file);

    hoop.threadLength = binaryReadInt32(file); /* yes, it seems this is _not_ big endian */
    hoop.unknown2 = binaryReadByte(file);
    hoop.numberOfColors = binaryReadByte(file);
    hoop.unknown3 = binaryReadInt16BE(file);
    hoop.unknown4 = binaryReadInt32BE(file);
    hoop.numberOfBytesRemaining = binaryReadInt32BE(file);

    hoop.xOffset = binaryReadInt32BE(file);
    hoop.yOffset = binaryReadInt32BE(file);

    hoop.byte1 = binaryReadByte(file);
    hoop.byte2 = binaryReadByte(file);
    hoop.byte3 = binaryReadByte(file);

    /* Centered hoop dimensions */
    hoop.right2 = binaryReadInt32BE(file);
    hoop.left2 = binaryReadInt32BE(file);
    hoop.bottom2 = binaryReadInt32BE(file);
    hoop.top2 = binaryReadInt32BE(file);

    hoop.width = binaryReadInt32BE(file);
    hoop.height = binaryReadInt32BE(file);
    return hoop;
}

/*! Reads a file with the given \a fileName and loads the data into \a pattern.
 *  Returns \c true if successful, otherwise returns \c false. */
static int readVp3(EmbPattern* pattern, EmbFile* file, const char* fileName)
{
    unsigned char magicString[5];
    unsigned char some;
    unsigned char* softwareVendorString = 0;
    unsigned char v1;
    /* unsigned char v2, ..., v18; */
    unsigned char* anotherSoftwareVendorString = 0;
    int numberOfColors;
    long colorSectionOffset;
    unsigned char magicCode[6];
    short someShort;
    unsigned char someByte;
    int bytesRemainingInFile;
    unsigned char* fileCommentString = 0; /* some software writes used settings here */
    int hoopConfigurationOffset;
    unsigned char* anotherCommentString = 0;
    int i;

    binaryReadBytes(file, magicString, 5); /* %vsm% */ /* TODO: check return value */

    some = binaryReadByte(file); /* 0 */
    softwareVendorString = vp3ReadString(file);
    someShort = binaryReadInt16(file);
    someByte = binaryReadByte(file);
    bytesRemainingInFile = binaryReadInt32(file);
    fileCommentString = vp3ReadString(file);
    hoopConfigurationOffset = (int)embFile_tell(file);

    vp3ReadHoopSection(file);

    anotherCommentString = vp3ReadString(file);

    /* TODO: review v1 thru v18 variables and use emb_unused() if needed */
    for (i = 0; i < 18; i++) {
        v1 = binaryReadByte(file);
    }

    binaryReadBytes(file, magicCode, 6); /* 0x78 0x78 0x55 0x55 0x01 0x00 */ /* TODO: check return value */

    anotherSoftwareVendorString = vp3ReadString(file);

    numberOfColors = binaryReadInt16BE(file);
    embLog("ERROR: format-vp3.c Number of Colors: %d\n" /*, numberOfColors */);
    colorSectionOffset = (int)embFile_tell(file);

    for (i = 0; i < numberOfColors; i++) {
        EmbThread t;
        char tableSize;
        int startX, startY, offsetToNextColorX, offsetToNextColorY;
        unsigned char *threadColorNumber, *colorName, *threadVendor;
        int unknownThreadString, numberOfBytesInColor;

        embFile_seek(file, colorSectionOffset, SEEK_SET);
        embLog("ERROR: format-vp3.c Color Check Byte #1: 0 == %d\n" /*, binaryReadByte(file)*/);
        embLog("ERROR: format-vp3.c Color Check Byte #2: 5 == %d\n" /*, binaryReadByte(file)*/);
        embLog("ERROR: format-vp3.c Color Check Byte #3: 0 == %d\n" /*, binaryReadByte(file)*/);
        colorSectionOffset = binaryReadInt32BE(file);
        colorSectionOffset += embFile_tell(file);
        startX = binaryReadInt32BE(file);
        startY = binaryReadInt32BE(file);
        embPattern_addStitchAbs(pattern, startX / 1000.0, -startY / 1000.0, JUMP, 1);

        tableSize = binaryReadByte(file);
        binaryReadByte(file);
        t.color.r = binaryReadByte(file);
        t.color.g = binaryReadByte(file);
        t.color.b = binaryReadByte(file);
        embPattern_addThread(pattern, t);
        embFile_seek(file, 6 * tableSize - 1, SEEK_CUR);

        threadColorNumber = vp3ReadString(file);
        colorName = vp3ReadString(file);
        threadVendor = vp3ReadString(file);

        offsetToNextColorX = binaryReadInt32BE(file);
        offsetToNextColorY = binaryReadInt32BE(file);

        unknownThreadString = binaryReadInt16BE(file);
        embFile_seek(file, unknownThreadString, SEEK_CUR);
        numberOfBytesInColor = binaryReadInt32BE(file);
        embFile_seek(file, 0x3, SEEK_CUR);
        while (embFile_tell(file) < colorSectionOffset - 1) {
            int lastFilePosition = embFile_tell(file);

            int x = vp3Decode(binaryReadByte(file));
            int y = vp3Decode(binaryReadByte(file));
            if (x == 0x80) {
                switch (y) {
                case 0x00:
                case 0x03:
                    break;
                case 0x01:
                    x = vp3DecodeInt16(binaryReadUInt16BE(file));
                    y = vp3DecodeInt16(binaryReadUInt16BE(file));
                    binaryReadInt16BE(file);
                    embPattern_addStitchRel(pattern, x / 10.0, y / 10.0, TRIM, 1);
                    break;
                default:
                    break;
                }
            } else {
                embPattern_addStitchRel(pattern, x / 10.0, y / 10.0, NORMAL, 1);
            }

            if (embFile_tell(file) == lastFilePosition) {
                embLog("ERROR: format-vp3.c could not read stitch block in entirety\n");
                return 0;
            }
        }
        if (i + 1 < numberOfColors)
            embPattern_addStitchRel(pattern, 0, 0, STOP, 1);
    }

    embPattern_flipVertical(pattern);

    return 1;
}

void vp3WriteStringLen(EmbFile* file, const char* str, int len)
{
    binaryWriteUShortBE(file, len);
    binaryWriteBytes(file, str, len);
}

void vp3WriteString(EmbFile* file, const char* str)
{
    vp3WriteStringLen(file, str, strlen(str));
}

void vp3PatchByteCount(EmbFile* file, int offset, int adjustment)
{
    int currentPos = embFile_tell(file);
    embFile_seek(file, offset, SEEK_SET);
    embLog("Patching byte count: \n" /*, currentPos - offset + adjustment */);
    binaryWriteIntBE(file, currentPos - offset + adjustment);
    embFile_seek(file, currentPos, SEEK_SET);
}

/*! Writes the data from \a pattern to a file with the given \a fileName.
 *  Returns \c true if successful, otherwise returns \c false. */
static int writeVp3(EmbPattern* pattern, EmbFile* file, const char* fileName)
{
    EmbRect bounds;
    int remainingBytesPos, remainingBytesPos2;
    int colorSectionStitchBytes;
    int first = 1, i;
    int numberOfColors = 0;
    EmbColor color = { 0xFE, 0xFE, 0xFE };
    EmbStitch st;

    bounds = embPattern_calcBoundingBox(pattern);

    embPattern_correctForMaxStitchLength(pattern, 3200.0, 3200.0); /* VP3 can encode signed 16bit deltas */

    embPattern_flipVertical(pattern);

    binaryWriteBytes(file, "%vsm%", 5);
    binaryWriteByte(file, 0);
    vp3WriteString(file, "Embroidermodder");
    binaryWriteByte(file, 0);
    binaryWriteByte(file, 2);
    binaryWriteByte(file, 0);

    remainingBytesPos = embFile_tell(file);
    binaryWriteInt(file, 0); /* placeholder */
    vp3WriteString(file, "");
    binaryWriteIntBE(file, bounds.right * 1000);
    binaryWriteIntBE(file, bounds.bottom * 1000);
    binaryWriteIntBE(file, bounds.left * 1000);
    binaryWriteIntBE(file, bounds.top * 1000);
    binaryWriteInt(file, 0); /* this would be some (unknown) function of thread length */
    binaryWriteByte(file, 0);

    numberOfColors = 0;

    int flag;
    EmbColor newColor;
    for (i = 0; i < pattern->stitchList->count; i++) {
        st = pattern->stitchList->stitch[i];

        /* pointer = mainPointer; */
        flag = st.flags;
        newColor = pattern->threads->thread[st.color].color;
        if (newColor.r != color.r || newColor.g != color.g || newColor.b != color.b) {
            numberOfColors++;
            color.r = newColor.r;
            color.g = newColor.g;
            color.b = newColor.b;
        } else if (flag & END || flag & STOP) {
            numberOfColors++;
        }

        while (flag == pattern->stitchList->stitch[i].flags) {
            i++;
        }
        /* mainPointer = pointer; */
    }

    binaryWriteByte(file, numberOfColors);
    binaryWriteByte(file, 12);
    binaryWriteByte(file, 0);
    binaryWriteByte(file, 1);
    binaryWriteByte(file, 0);
    binaryWriteByte(file, 3);
    binaryWriteByte(file, 0);

    remainingBytesPos2 = embFile_tell(file);
    binaryWriteInt(file, 0); /* placeholder */

    binaryWriteIntBE(file, 0); /* origin X */
    binaryWriteIntBE(file, 0); /* origin Y */
    binaryWriteByte(file, 0);
    binaryWriteByte(file, 0);
    binaryWriteByte(file, 0);

    binaryWriteIntBE(file, bounds.right * 1000);
    binaryWriteIntBE(file, bounds.bottom * 1000);
    binaryWriteIntBE(file, bounds.left * 1000);
    binaryWriteIntBE(file, bounds.top * 1000);

    binaryWriteIntBE(file, (bounds.right - bounds.left) * 1000);
    binaryWriteIntBE(file, (bounds.bottom - bounds.top) * 1000);

    vp3WriteString(file, "");
    binaryWriteShortBE(file, 25700);
    binaryWriteIntBE(file, 4096);
    binaryWriteIntBE(file, 0);
    binaryWriteIntBE(file, 0);
    binaryWriteIntBE(file, 4096);

    binaryWriteBytes(file, "xxPP\x01\0", 6);
    vp3WriteString(file, "");
    binaryWriteShortBE(file, numberOfColors);

    for (i = 0; i < pattern->stitchList->count; i++) {
        char colorName[8] = { 0 };
        double lastX, lastY;
        int colorSectionLengthPos;
        EmbStitch s;
        int lastColor;

        if (!first) {
            binaryWriteByte(file, 0);
        }
        binaryWriteByte(file, 0);
        binaryWriteByte(file, 5);
        binaryWriteByte(file, 0);

        colorSectionLengthPos = embFile_tell(file);
        binaryWriteInt(file, 0); /* placeholder */

        /* pointer = mainPointer; */
        color = pattern->threads->thread[st.color].color;

        if (first && (st.flags & JUMP) && pattern->stitchList->stitch[i + 1].flags & JUMP) {
            i++;
        }

        s = pattern->stitchList->stitch[i];
        embLog("format-vp3.c DEBUG %d, %lf, %lf\n" /*, s.flags, s.x, s.y */);
        binaryWriteIntBE(file, s.x * 1000);
        binaryWriteIntBE(file, -s.y * 1000);
        /* pointer = pointer->next; */

        first = 0;

        lastX = s.x;
        lastY = s.y;
        lastColor = s.color;

        binaryWriteByte(file, 1);
        binaryWriteByte(file, 0);

        embLog("format-vp3.c writeVp3(), switching to color (%d, %d, %d)\n" /*, color.r, color.g, color.b */);
        binaryWriteByte(file, color.r);
        binaryWriteByte(file, color.g);
        binaryWriteByte(file, color.b);

        binaryWriteByte(file, 0);
        binaryWriteByte(file, 0);
        binaryWriteByte(file, 0);
        binaryWriteByte(file, 5);
        binaryWriteByte(file, 40);

        vp3WriteString(file, "");

        sprintf(colorName, "#%02x%02x%02x", color.b, color.g, color.r);

        vp3WriteString(file, colorName);
        vp3WriteString(file, "");

        binaryWriteIntBE(file, 0);
        binaryWriteIntBE(file, 0);

        vp3WriteStringLen(file, "\0", 1);

        colorSectionStitchBytes = embFile_tell(file);
        binaryWriteInt(file, 0); /* placeholder */

        binaryWriteByte(file, 10);
        binaryWriteByte(file, 246);
        binaryWriteByte(file, 0);

        for (i = 0; i < pattern->stitchList->count; i++) {
            int dx, dy;

            EmbStitch s = pattern->stitchList->stitch[i];
            if (s.color != lastColor) {
                break;
            }
            if (s.flags & END || s.flags & STOP) {
                break;
            }
            dx = (s.x - lastX) * 10;
            dy = (s.y - lastY) * 10;
            lastX = lastX + dx / 10.0; /* output is in ints, ensure rounding errors do not sum up */
            lastY = lastY + dy / 10.0;

            if (dx < -127 || dx > 127 || dy < -127 || dy > 127) {
                binaryWriteByte(file, 128);
                binaryWriteByte(file, 1);
                binaryWriteShortBE(file, dx);
                binaryWriteShortBE(file, dy);
                binaryWriteByte(file, 128);
                binaryWriteByte(file, 2);
            } else {
                binaryWriteByte(file, dx);
                binaryWriteByte(file, dy);
            }
        }

        vp3PatchByteCount(file, colorSectionStitchBytes, -4);
        vp3PatchByteCount(file, colorSectionLengthPos, -3);
        /* mainPointer = pointer; */
    }

    vp3PatchByteCount(file, remainingBytesPos2, -4);
    vp3PatchByteCount(file, remainingBytesPos, -4);

    embPattern_flipVertical(pattern);

    return 1;
}

