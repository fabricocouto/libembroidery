static int exyDecodeFlags(unsigned char b2)
{
    int returnCode = 0;
    if (b2 == 0xF3)
        return (END);
    if ((b2 & 0xC3) == 0xC3)
        return TRIM | STOP;
    if (b2 & 0x80)
        returnCode |= TRIM;
    if (b2 & 0x40)
        returnCode |= STOP;
    return returnCode;
}

/*! Reads a file with the given \a fileName and loads the data into \a pattern.
 *  Returns \c true if successful, otherwise returns \c false. */
static int readExy(EmbPattern* pattern, EmbFile* file, const char* fileName)
{
    unsigned char b[3];

    embPattern_loadExternalColorFile(pattern, fileName);

    embFile_seek(file, 0x100, SEEK_SET);

    while (embFile_read(b, 1, 3, file) == 3) {
        int flags;
        int x = 0;
        int y = 0;
        if (b[0] & 0x01)
            x += 1;
        if (b[0] & 0x02)
            x -= 1;
        if (b[0] & 0x04)
            x += 9;
        if (b[0] & 0x08)
            x -= 9;
        if (b[0] & 0x80)
            y += 1;
        if (b[0] & 0x40)
            y -= 1;
        if (b[0] & 0x20)
            y += 9;
        if (b[0] & 0x10)
            y -= 9;
        if (b[1] & 0x01)
            x += 3;
        if (b[1] & 0x02)
            x -= 3;
        if (b[1] & 0x04)
            x += 27;
        if (b[1] & 0x08)
            x -= 27;
        if (b[1] & 0x80)
            y += 3;
        if (b[1] & 0x40)
            y -= 3;
        if (b[1] & 0x20)
            y += 27;
        if (b[1] & 0x10)
            y -= 27;
        if (b[2] & 0x04)
            x += 81;
        if (b[2] & 0x08)
            x -= 81;
        if (b[2] & 0x20)
            y += 81;
        if (b[2] & 0x10)
            y -= 81;
        flags = exyDecodeFlags(b[2]);
        if ((flags & END) == END) {
            embPattern_addStitchRel(pattern, 0, 0, END, 1);
            break;
        }
        embPattern_addStitchRel(pattern, x / 10.0, y / 10.0, flags, 1);
    }

    embPattern_end(pattern);

    return 1;
}

/*! Writes the data from \a pattern to a file with the given \a fileName.
 *  Returns \c true if successful, otherwise returns \c false. */
static int writeExy(EmbPattern* pattern, EmbFile* file, const char* fileName)
{
    return 0; /*TODO: finish writeExy */
}

