/*! Reads a file with the given \a fileName and loads the data into \a pattern.
 *  Returns \c true if successful, otherwise returns \c false. */
static int readPes(EmbPattern* pattern, EmbFile* file, const char* fileName)
{
    int pecstart, numColors, x;

    embFile_seek(file, 8, SEEK_SET);
    pecstart = binaryReadInt32(file);

    embFile_seek(file, pecstart + 48, SEEK_SET);
    numColors = embFile_getc(file) + 1;
    for (x = 0; x < numColors; x++) {
        embPattern_addThread(pattern, pecThreads[(unsigned char)embFile_getc(file)]);
    }

    embFile_seek(file, pecstart + 532, SEEK_SET);
    readPecStitches(pattern, file, fileName);

    embPattern_flipVertical(pattern);

    return 1;
}

/* static void readPESHeader(EmbFile* file, EmbPattern* pattern) {
       char* signature = readString(8);
            if(strcmp(signature, "#PES0100") {
                readPESHeaderV10();
            } else if(strcmp(signature, "#PES0090") {
                readPESHeaderV9();
            } else if(strcmp(signature, "#PES0080") {
                readPESHeaderV8();
            } else if(strcmp(signature, "#PES0070") {
                readPESHeaderV7();
            } else if(strcmp(signature, "#PES0060") {
                readPESHeaderV6();
            } else if(strcmp(signature, "#PES0056"){
                readPESHeaderV5();
            } else if(strcmp(signature, "#PES0055") {
               readPESHeaderV5();
                 } else if(strcmp(signature, "#PES0050") {
                readPESHeaderV5();
                 } else if(strcmp(signature, "#PES0040") {
                readPESHeaderV4();
                 } else if(strcmp(signature, "#PES0030") {
                readPESHeaderDefault();
                 } else if(strcmp(signature, "#PES0022") {
                readPESHeaderDefault();
                 } else if(strcmp(signature, "#PES0020") {
                readPESHeaderDefault();
                 } else if(strcmp(signature, "#PES0001") {
                readPESHeaderDefault();
                 } else
            if(strcmp(signature, "#PEC0001") {
                //PEC needs to go straight to reading, no default.
            } else {
                readPESHeaderDefault();
            }
        }
    }

static void readPESHeaderDefault()  {
    int pecStart = readInt32LE();
    skip(pecStart - readPosition);
}

    static void readDescriptions()  {
        int DesignStringLength = readInt8();
        String DesignName = readString(DesignStringLength);
        pattern.setName(DesignName);
        int categoryStringLength = readInt8();
        String Category = readString(categoryStringLength);
        pattern.setCategory(Category);
        int authorStringLength = readInt8();
        String Author = readString(authorStringLength);
        pattern.setAuthor(Author);
        int keywordsStringLength = readInt8();
        String keywords = readString(keywordsStringLength);
        pattern.setKeywords(keywords);
        int commentsStringLength = readInt8();
        String Comments = readString(commentsStringLength);
        pattern.setComments(Comments);
    }

    static void readPESHeaderV4()  {
        int pecStart = readInt32LE();
        skip(4);
        readDescriptions();
        skip(pecStart - readPosition);
    }

    static void readPESHeaderV5()  {
        int pecStart = readInt32LE();
        skip(4);
        readDescriptions();
        skip(24);//36 v6
        int fromImageStringLength = readInt8();
        skip(fromImageStringLength);
        skip(24);
        int numberOfProgrammableFillPatterns = readInt16LE();
        if (numberOfProgrammableFillPatterns != 0) {
            seek(pecStart);
            return;
        }
        int numberOfMotifPatterns = readInt16LE();
        if (numberOfMotifPatterns != 0) {
            seek(pecStart);
            return;
        }
        int featherPatternCount = readInt16LE();
        if (featherPatternCount != 0) {
            seek(pecStart);
            return;
        }
        int numberOfColors = readInt16LE();
        for (int i = 0; i < numberOfColors; i++) {
            readThread();
        }
        seek(pecStart);
    }

    static void readPESHeaderV6()  {
        int pecStart = readInt32LE();
        skip(4);
        readDescriptions();
        skip(36);
        int fromImageStringLength = readInt8();
        String fromImageString = readString(fromImageStringLength);
        if (fromImageString.length() != 0) {
            pattern.setMetadata("image_file", fromImageString);
        }
        skip(24);
        int numberOfProgrammableFillPatterns = readInt16LE();
        if (numberOfProgrammableFillPatterns != 0) {
            seek(pecStart);
            return;
        }
        int numberOfMotifPatterns = readInt16LE();
        if (numberOfMotifPatterns != 0) {
            seek(pecStart);
            return;
        }
        int featherPatternCount = readInt16LE();
        if (featherPatternCount != 0) {
            seek(pecStart);
            return;
        }
        int numberOfColors = readInt16LE();
        for (int i = 0; i < numberOfColors; i++) {
            readThread();
        }
        seek(pecStart);
    }
    
    static void readPESHeaderV7()  {
        int pecStart = readInt32LE();
        skip(4);
        readDescriptions();
        skip(36);
        int fromImageStringLength = readInt8();
        String fromImageString = readString(fromImageStringLength);
        if (fromImageString.length() != 0) {
            pattern.setMetadata("image_file", fromImageString);
        }
        skip(24);
        int numberOfProgrammableFillPatterns = readInt16LE();
        if (numberOfProgrammableFillPatterns != 0) {
            seek(pecStart);
            return;
        }
        int numberOfMotifPatterns = readInt16LE();
        if (numberOfMotifPatterns != 0) {
            seek(pecStart);
            return;
        }
        int featherPatternCount = readInt16LE();
        if (featherPatternCount != 0) {
            seek(pecStart);
            return;
        }
        int numberOfColors = readInt16LE();
        for (int i = 0; i < numberOfColors; i++) {
            readThread();
        }
        seek(pecStart);
    }
    
    static void readPESHeaderV8()  {
        int pecStart = readInt32LE();
        skip(4);
        readDescriptions();
        skip(38);
        int fromImageStringLength = readInt8();
        String fromImageString = readString(fromImageStringLength);
        if (fromImageString.length() != 0) {
            pattern.setMetadata("image_file", fromImageString);
        }
        skip(26);
        int numberOfProgrammableFillPatterns = readInt16LE();
        if (numberOfProgrammableFillPatterns != 0) {
            seek(pecStart);
            return;
        }
        int numberOfMotifPatterns = readInt16LE();
        if (numberOfMotifPatterns != 0) {
            seek(pecStart);
            return;
        }
        int featherPatternCount = readInt16LE();
        if (featherPatternCount != 0) {
            seek(pecStart);
            return;
        }
        int numberOfColors = readInt16LE();
        for (int i = 0; i < numberOfColors; i++) {
            readThread();
        }
        seek(pecStart);
    }

    static void readPESHeaderV9()  {
        int pecStart = readInt32LE();
        skip(4);
        readDescriptions();
        skip(14);
        int hoopNameStringLength = readInt8();
        String hoopNameString = readString(hoopNameStringLength);
        if (hoopNameString.length() != 0) {
            pattern.setMetadata("hoop_name", hoopNameString);
        }
        skip(30);
        int fromImageStringLength = readInt8();
        String fromImageString = readString(fromImageStringLength);
        if (fromImageString.length() != 0) {
            pattern.setMetadata("image_file", fromImageString);
        }
        skip(34);
        int numberOfProgrammableFillPatterns = readInt16LE();
        if (numberOfProgrammableFillPatterns != 0) {
            seek(pecStart);
            return;
        }
        int numberOfMotifPatterns = readInt16LE();
        if (numberOfMotifPatterns != 0) {
            seek(pecStart);
            return;
        }
        int featherPatternCount = readInt16LE();
        if (featherPatternCount != 0) {
            seek(pecStart);
            return;
        }
        int numberOfColors = readInt16LE();
        for (int i = 0; i < numberOfColors; i++) {
            readThread();
        }
        seek(pecStart);
    }

    static void readPESHeaderV10()  {
        int pecStart = readInt32LE();
        skip(4);
        readDescriptions();
        skip(14);
        int hoopNameStringLength = readInt8();
        String hoopNameString = readString(hoopNameStringLength);
        if (hoopNameString.length() != 0) {
            pattern.setMetadata("hoop_name", hoopNameString);
        }
        skip(38);
        int fromImageStringLength = readInt8();
        String fromImageString = readString(fromImageStringLength);
        if (fromImageString.length() != 0) {
            pattern.setMetadata("image_file", fromImageString);
        }
        skip(34);
        int numberOfProgrammableFillPatterns = readInt16LE();
        if (numberOfProgrammableFillPatterns != 0) {
            seek(pecStart);
            return;
        }
        int numberOfMotifPatterns = readInt16LE();
        if (numberOfMotifPatterns != 0) {
            seek(pecStart);
            return;
        }
        int featherPatternCount = readInt16LE();
        if (featherPatternCount != 0) {
            seek(pecStart);
            return;
        }
        int numberOfColors = readInt16LE();
        for (int i = 0; i < numberOfColors; i++) {
            readThread();
        }
        seek(pecStart);
    }
    
    static void readThread() {
        int color_code_length = readInt8();
        String color_code = readString(color_code_length);
        int red = readInt8();
        int green = readInt8();
        int blue = readInt8();
        skip(5);
        int descriptionStringLength = readInt8();
        String description = readString(descriptionStringLength);

        int brandStringLength = readInt8();
        String brand = readString(brandStringLength);

        int threadChartStringLength = readInt8();
        String threadChart = readString(threadChartStringLength);

        int color = (red & 0xFF) << 16 | (green & 0xFF) << 8 | (blue & 0xFF);
        pattern.add(new EmbThread(color, description, color_code, brand, threadChart));
    }
*/
static void pesWriteSewSegSection(EmbPattern* pattern, EmbFile* file, const char* fileName)
{
    /* TODO: pointer safety */
    short* colorInfo = 0;
    int flag = 0;
    int count = 0;
    int colorCode = -1;
    int stitchType = 0;
    int blockCount = 0;
    int colorCount = 0;
    int newColorCode = 0;
    int colorInfoIndex = 0;
    int i;
    EmbRect bounds = embPattern_calcBoundingBox(pattern);
    EmbColor color;
    EmbStitch st;

    /* TODO: why is this repeated below? */
    /*
    for (i=0; i<pattern->stitchList->count; i++) {
        j = i;
        flag = st.flags;
        color = pattern->threads->thread[st.color].color;
        newColorCode = embThread_findNearestColor_fromThread(color, (EmbThread*)pecThreads, pecThreadCount);
        if (newColorCode != colorCode) {
            colorCount++;
            colorCode = newColorCode;
        }
        while (i<pattern->stitchList->count && (flag == st.flags)) {
            st = pattern->stitchList->stitch[i];
            count++;
            i++;
        }
        blockCount++;
        i = j;
    }
    */

    binaryWriteShort(file, (short)blockCount); /* block count */
    binaryWriteUShort(file, 0xFFFF);
    binaryWriteShort(file, 0x00);

    binaryWriteShort(file, 0x07); /* string length */
    binaryWriteBytes(file, "CSewSeg", 7);

    colorInfo = (short*)calloc(colorCount * 2, sizeof(short));

    colorCode = -1;
    blockCount = 0;
    for (i = 0; i < pattern->stitchList->count; i++) {
        st = pattern->stitchList->stitch[i];
        flag = st.flags;
        color = pattern->threads->thread[st.color].color;
        newColorCode = embThread_findNearestColor_fromThread(color, (EmbThread*)pecThreads, pecThreadCount);
        if (newColorCode != colorCode) {
            colorInfo[colorInfoIndex++] = (short)blockCount;
            colorInfo[colorInfoIndex++] = (short)newColorCode;
            colorCode = newColorCode;
        }
        /* TODO: check if this has an off-by-one error */
        for (count = 0; flag == st.flags; count++) {
            st = pattern->stitchList->stitch[i + count];
        }

        /* 1 for jump, 0 for normal */
        stitchType = flag & JUMP;
        binaryWriteShort(file, (short)stitchType);
        /* color code */
        binaryWriteShort(file, (short)colorCode);
        /* stitches in block */
        binaryWriteShort(file, (short)count);
        for (; flag == st.flags && i < pattern->stitchList->count; i++) {
            st = pattern->stitchList->stitch[i];
            binaryWriteShort(file, (short)(st.x - bounds.left));
            binaryWriteShort(file, (short)(st.y + bounds.top));
        }
        if (i < pattern->stitchList->count) {
            binaryWriteUShort(file, 0x8003);
        }
        blockCount++;
    }
    binaryWriteShort(file, (short)colorCount);
    for (i = 0; i < colorCount; i++) {
        binaryWriteShort(file, colorInfo[i * 2]);
        binaryWriteShort(file, colorInfo[i * 2 + 1]);
    }
    binaryWriteInt(file, 0);
    if (colorInfo) {
        free(colorInfo);
        colorInfo = 0;
    }
}

static void pesWriteEmbOneSection(EmbPattern* pattern, EmbFile* file, const char* fileName)
{
    /* TODO: pointer safety */
    int i;
    int hoopHeight = 1800, hoopWidth = 1300;
    EmbRect bounds;
    binaryWriteShort(file, 0x07); /* string length */
    binaryWriteBytes(file, "CEmbOne", 7);
    bounds = embPattern_calcBoundingBox(pattern);

    binaryWriteShort(file, 0);
    binaryWriteShort(file, 0);
    binaryWriteShort(file, 0);
    binaryWriteShort(file, 0);

    binaryWriteShort(file, 0);
    binaryWriteShort(file, 0);
    binaryWriteShort(file, 0);
    binaryWriteShort(file, 0);

    /* AffineTransform */
    binaryWriteFloat(file, 1.0f);
    binaryWriteFloat(file, 0.0f);
    binaryWriteFloat(file, 0.0f);
    binaryWriteFloat(file, 1.0f);
    binaryWriteFloat(file, (float)((embRect_width(bounds) - hoopWidth) / 2));
    binaryWriteFloat(file, (float)((embRect_height(bounds) + hoopHeight) / 2));

    binaryWriteShort(file, 1);
    binaryWriteShort(file, 0); /* Translate X */
    binaryWriteShort(file, 0); /* Translate Y */
    binaryWriteShort(file, (short)embRect_width(bounds));
    binaryWriteShort(file, (short)embRect_height(bounds));

    for (i = 0; i < 8; i++) {
        binaryWriteByte(file, 0);
    }

    /*WriteSubObjects(br, pes, SubBlocks); */
}

/*! Writes the data from \a pattern to a file with the given \a fileName.
 *  Returns \c true if successful, otherwise returns \c false. */
static int writePes(EmbPattern* pattern, EmbFile* file, const char* fileName)
{
    int pecLocation;

    embPattern_flipVertical(pattern);
    embPattern_scale(pattern, 10.0);
    binaryWriteBytes(file, "#PES0001", 8);
    /* WRITE PECPointer 32 bit int */
    binaryWriteInt(file, 0x00);

    binaryWriteShort(file, 0x01);
    binaryWriteShort(file, 0x01);

    /* Write object count */
    binaryWriteUShort(file, 0x01);
    binaryWriteUShort(file, 0xFFFF); /* command */
    binaryWriteUShort(file, 0x00); /* unknown */

    pesWriteEmbOneSection(pattern, file, fileName);
    pesWriteSewSegSection(pattern, file, fileName);

    pecLocation = embFile_tell(file);
    embFile_seek(file, 0x08, SEEK_SET);
    binaryWriteByte(file, (unsigned char)(pecLocation & 0xFF));
    binaryWriteByte(file, (unsigned char)(pecLocation >> 8) & 0xFF);
    binaryWriteByte(file, (unsigned char)(pecLocation >> 16) & 0xFF);
    embFile_seek(file, 0x00, SEEK_END);
    writePecStitches(pattern, file, fileName);
    return 1;
}

