/*! Reads a file with the given \a fileName and loads the data into \a pattern.
 *  Returns \c true if successful, otherwise returns \c false. */
static int readDsb(EmbPattern* pattern, EmbFile* file, const char* fileName)
{
    unsigned char header[512];
    embPattern_loadExternalColorFile(pattern, fileName);
    embFile_read(header, 1, 512, file);

    embFile_seek(file, 0x200, SEEK_SET);
    while (1) {
        int x, y;
        unsigned char ctrl;
        int stitchType = NORMAL;

        ctrl = (unsigned char)embFile_getc(file);
        if (embFile_eof(file))
            break;
        y = embFile_getc(file);
        if (embFile_eof(file))
            break;
        x = embFile_getc(file);
        if (embFile_eof(file))
            break;
        if (ctrl & 0x01)
            stitchType = TRIM;
        if (ctrl & 0x20)
            x = -x;
        if (ctrl & 0x40)
            y = -y;
        /* ctrl & 0x02 - Speed change? */ /* TODO: review this line */
        /* ctrl & 0x04 - Clutch? */ /* TODO: review this line */
        if ((ctrl & 0x05) == 0x05) {
            stitchType = STOP;
        }
        if (ctrl == 0xF8 || ctrl == 0x91 || ctrl == 0x87) {
            embPattern_end(pattern);
            break;
        }
        embPattern_addStitchRel(pattern, x / 10.0, y / 10.0, stitchType, 1);
    }
    return 1;
}

/*! Writes the data from \a pattern to a file with the given \a fileName.
 *  Returns \c true if successful, otherwise returns \c false. */
static int writeDsb(EmbPattern* pattern, EmbFile* file, const char* fileName)
{
    return 0; /*TODO: finish writeDsb */
}

