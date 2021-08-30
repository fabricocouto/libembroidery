#include "embroidery.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#ifdef ARDUINO
#include "utility/ino-event.h"
#endif

static EmbColor black = {0,0,0};

/*! Returns a pointer to an EmbPattern. It is created on the heap.
 * The caller is responsible for freeing the allocated memory with
 * embPattern_free(). */
EmbPattern* embPattern_create(void)
{
    EmbPattern* p = 0;
    p = (EmbPattern*)malloc(sizeof(EmbPattern));
    if(!p) { embLog_error("emb-pattern.c embPattern_create(), unable to allocate memory for p\n"); return 0; }

    p->settings = embSettings_init();
    p->currentColorIndex = 0;
    p->stitchList = 0;
    p->threads = 0;
    p->hoop.height = 0.0;
    p->hoop.width = 0.0;
    p->arcs = 0;
    p->circles = 0;
    p->ellipses = 0;
    p->lines = 0;
    p->paths = 0;
    p->points = 0;
    p->polygons = 0;
    p->polylines = 0;
    p->rects = 0;
    p->splines = 0;

    p->lastStitch = 0;

    p->lastX = 0.0;
    p->lastY = 0.0;

    return p;
}

void embPattern_hideStitchesOverLength(EmbPattern* p, int length)
{
    double prevX = 0;
    double prevY = 0;
    EmbStitchList* pointer = 0;

    if(!p) { embLog_error("emb-pattern.c embPattern_hideStitchesOverLength(), p argument is null\n"); return; }
    pointer = p->stitchList;
    while(pointer)
    {
        if((fabs(pointer->stitch.x - prevX) > length) || (fabs(pointer->stitch.y - prevY) > length))
        {
            pointer->stitch.flags |= TRIM;
            pointer->stitch.flags &= ~NORMAL;
        }
        prevX = pointer->stitch.x;
        prevY = pointer->stitch.y;
        pointer = pointer->next;
    }
}

int embPattern_addThread(EmbPattern* p, EmbThread thread)
{
    if (!p) {
        embLog_error("emb-pattern.c embPattern_addThread(), p argument is null\n");
        return 0;
    }
    if (!p->threads) {
        p->threads = embArray_create(EMB_THREAD);
    }
    embArray_addThread(p->threads, thread);
    return 1;
}

void embPattern_fixColorCount(EmbPattern* p)
{
    /* fix color count to be max of color index. */
    int maxColorIndex = 0;
    EmbStitchList* list = 0;

    if (!p) {
        embLog_error("emb-pattern.c embPattern_fixColorCount(), p argument is null\n");
        return;
    }
    list = p->stitchList;
    while (list) {
        maxColorIndex = embMaxInt(maxColorIndex, list->stitch.color);
        list = list->next;
    }
    while (p->threads->count <= maxColorIndex) {
        embPattern_addThread(p, embThread_getRandom());
    }
    /*
    while (p->threadLists->count > (maxColorIndex + 1)) {
        TODO: erase last color    p->threadList.pop_back();
    }
    */
}

/*! Copies all of the EmbStitchList data to EmbPolylineObjectList data for pattern (\a p). */
void embPattern_copyStitchListToPolylines(EmbPattern* p)
{
    EmbStitchList* stList;
    EmbPointObject* point;
    int breakAtFlags;

    if(!p) { embLog_error("emb-pattern.c embPattern_copyStitchListToPolylines(), p argument is null\n"); return; }

#ifdef EMB_DEBUG_JUMP
    breakAtFlags = (STOP | TRIM);
#else /* EMB_DEBUG_JUMP */
    breakAtFlags = (STOP | JUMP | TRIM);
#endif /* EMB_DEBUG_JUMP */

    stList = p->stitchList;
    while(stList)
    {
        EmbArray *pointList = 0;
        EmbColor color;
        while(stList)
        {
            if(stList->stitch.flags & breakAtFlags)
            {
                break;
            }
            if(!(stList->stitch.flags & JUMP))
            {
                if (!pointList)
                {
                    pointList = embArray_create(EMB_POINT);
                    color = p->threads->thread[stList->stitch.color].color;
                }
                point = (EmbPointObject *) malloc(sizeof(EmbPointObject));
                point->point.x = stList->stitch.x;
                point->point.y = stList->stitch.y;
                embArray_addPoint(pointList, point);
            }
            stList = stList->next;
        }

        /* NOTE: Ensure empty polylines are not created. This is critical. */
        if(pointList)
        {
            EmbPolylineObject* currentPolyline = (EmbPolylineObject*)malloc(sizeof(EmbPolylineObject));
            if(!currentPolyline) { embLog_error("emb-pattern.c embPattern_copyStitchListToPolylines(), cannot allocate memory for currentPolyline\n"); return; }
            currentPolyline->pointList = pointList;
            currentPolyline->color = color;
            currentPolyline->lineType = 1; /* TODO: Determine what the correct value should be */

            if (!p->polylines) {
                p->polylines = embArray_create(EMB_POLYLINE);
            }
            embArray_addPolyline(p->polylines, currentPolyline);
        }
        if(stList)
        {
            stList = stList->next;
        }
    }
}

/*! Copies all of the EmbPolylineObjectList data to EmbStitchList data for pattern (\a p). */
void embPattern_copyPolylinesToStitchList(EmbPattern* p)
{
    int firstObject = 1, i, j;
    /*int currentColor = polyList->polylineObj->color TODO: polyline color */

    if(!p) { embLog_error("emb-pattern.c embPattern_copyPolylinesToStitchList(), p argument is null\n"); return; }
    if (!p->polylines) { embLog_error("emb-pattern.c embPattern_copyPolylinesToStitchList(), p argument is null\n"); return; }
    for (i=0; i<p->polylines->count; i++) {
        EmbPolylineObject* currentPoly = 0;
        EmbArray* currentPointList = 0;
        EmbThread thread;

        currentPoly = p->polylines->polyline[i];
        if(!currentPoly) { embLog_error("emb-pattern.c embPattern_copyPolylinesToStitchList(), currentPoly is null\n"); return; }
        currentPointList = currentPoly->pointList;
        if(!currentPointList) { embLog_error("emb-pattern.c embPattern_copyPolylinesToStitchList(), currentPointList is null\n"); return; }

        thread.catalogNumber = 0;
        thread.color = currentPoly->color;
        thread.description = 0;
        embPattern_addThread(p, thread);

        if (!firstObject) {
            embPattern_addStitchAbs(p, currentPointList->point[0].point.x, currentPointList->point[0].point.y, TRIM, 1);
            embPattern_addStitchRel(p, 0.0, 0.0, STOP, 1);
        }

        embPattern_addStitchAbs(p, currentPointList->point[0].point.x, currentPointList->point[0].point.y, JUMP, 1);
        for (j=1; j<currentPointList->count; j++) { 
            embPattern_addStitchAbs(p, currentPointList->point[j].point.x, currentPointList->point[j].point.y, NORMAL, 1);
        }
        firstObject = 0;
    }
    embPattern_addStitchRel(p, 0.0, 0.0, END, 1);
}

/*! Moves all of the EmbStitchList data to EmbPolylineObjectList data for pattern (\a p). */
void embPattern_moveStitchListToPolylines(EmbPattern* p)
{
    if(!p) { embLog_error("emb-pattern.c embPattern_moveStitchListToPolylines(), p argument is null\n"); return; }
    embPattern_copyStitchListToPolylines(p);
    /* Free the stitchList and threadList since their data has now been transferred to polylines */
    embStitchList_free(p->stitchList);
    p->stitchList = 0;
    p->lastStitch = 0;
    /*embArray_free(p->threads);*/
}

/*! Moves all of the EmbPolylineObjectList data to EmbStitchList data for pattern (\a p). */
void embPattern_movePolylinesToStitchList(EmbPattern* p)
{
    if(!p) { embLog_error("emb-pattern.c embPattern_movePolylinesToStitchList(), p argument is null\n"); return; }
    embPattern_copyPolylinesToStitchList(p);
    embArray_free(p->polylines);
}

/*! Adds a stitch to the pattern (\a p) at the absolute position (\a x,\a y). Positive y is up. Units are in millimeters. */
void embPattern_addStitchAbs(EmbPattern* p, double x, double y, int flags, int isAutoColorIndex)
{
    EmbStitch s;

    if(!p) { embLog_error("emb-pattern.c embPattern_addStitchAbs(), p argument is null\n"); return; }

    if(flags & END)
    {
        if(embStitchList_empty(p->stitchList))
            return;
        /* Prevent unnecessary multiple END stitches */
        if(p->lastStitch->stitch.flags & END)
        {
            embLog_error("emb-pattern.c embPattern_addStitchAbs(), found multiple END stitches\n");
            return;
        }

        embPattern_fixColorCount(p);

        /* HideStitchesOverLength(127); TODO: fix or remove this */
    }

    if(flags & STOP)
    {
        if(embStitchList_empty(p->stitchList))
            return;
        if(isAutoColorIndex)
            p->currentColorIndex++;
    }

    /* NOTE: If the stitchList is empty, we will create it before adding stitches to it. The first coordinate will be the HOME position. */
    if(embStitchList_empty(p->stitchList))
    {
        /* NOTE: Always HOME the machine before starting any stitching */
        EmbPoint home = embSettings_home(&(p->settings));
        EmbStitch h;
        h.x = home.x;
        h.y = home.y;
        h.flags = JUMP;
        h.color = p->currentColorIndex;
        p->stitchList = p->lastStitch = embStitchList_create(h);
    }

    s.x = x;
    s.y = y;
    s.flags = flags;
    s.color = p->currentColorIndex;
#ifdef ARDUINO
    inoEvent_addStitchAbs(p, s.x, s.y, s.flags, s.color);
#else /* ARDUINO */
    p->lastStitch = embStitchList_add(p->lastStitch, s);
#endif /* ARDUINO */
    p->lastX = s.x;
    p->lastY = s.y;
}

/*! Adds a stitch to the pattern (\a p) at the relative position (\a dx,\a dy) to the previous stitch. Positive y is up. Units are in millimeters. */
void embPattern_addStitchRel(EmbPattern* p, double dx, double dy, int flags, int isAutoColorIndex)
{
    double x,y;

    if(!p) { embLog_error("emb-pattern.c embPattern_addStitchRel(), p argument is null\n"); return; }
    if(!embStitchList_empty(p->stitchList))
    {
        x = p->lastX + dx;
        y = p->lastY + dy;
    }
    else
    {
        /* NOTE: The stitchList is empty, so add it to the HOME position. The embStitchList_create function will ensure the first coordinate is at the HOME position. */
        EmbPoint home = embSettings_home(&(p->settings));
        x = home.x + dx;
        y = home.y + dy;
    }
    embPattern_addStitchAbs(p, x, y, flags, isAutoColorIndex);
}

void embPattern_changeColor(EmbPattern* p, int index)
{
    if(!p) { embLog_error("emb-pattern.c embPattern_changeColor(), p argument is null\n"); return; }
    p->currentColorIndex = index;
}

/*! Reads a file with the given \a fileName and loads the data into \a pattern.
 *  Returns \c true if successful, otherwise returns \c false. */
int embPattern_read(EmbPattern* pattern, const char* fileName) /* TODO: Write test case using this convenience function. */
{
    EmbReaderWriter* reader = 0;
    int result = 0;

    if(!pattern) { embLog_error("emb-pattern.c embPattern_read(), pattern argument is null\n"); return 0; }
    if(!fileName) { embLog_error("emb-pattern.c embPattern_read(), fileName argument is null\n"); return 0; }

    reader = embReaderWriter_getByFileName(fileName);
    if(!reader) { embLog_error("emb-pattern.c embPattern_read(), unsupported read file type: %s\n", fileName); return 0; }
    result = reader->reader(pattern, fileName);
    free(reader);
    reader = 0;
    return result;
}

/*! Writes the data from \a pattern to a file with the given \a fileName.
 *  Returns \c true if successful, otherwise returns \c false. */
int embPattern_write(EmbPattern* pattern, const char* fileName) /* TODO: Write test case using this convenience function. */
{
    EmbReaderWriter* writer = 0;
    int result = 0;

    if(!pattern) { embLog_error("emb-pattern.c embPattern_write(), pattern argument is null\n"); return 0; }
    if(!fileName) { embLog_error("emb-pattern.c embPattern_write(), fileName argument is null\n"); return 0; }

    writer = embReaderWriter_getByFileName(fileName);
    if(!writer) { embLog_error("emb-pattern.c embPattern_write(), unsupported write file type: %s\n", fileName); return 0; }
    result = writer->writer(pattern, fileName);
    free(writer);
    writer = 0;
    return result;
}

/* Very simple scaling of the x and y axis for every point.
* Doesn't insert or delete stitches to preserve density. */
void embPattern_scale(EmbPattern* p, double scale)
{
    EmbStitchList* pointer = 0;

    if(!p) { embLog_error("emb-pattern.c embPattern_scale(), p argument is null\n"); return; }
    pointer = p->stitchList;
    while(pointer)
    {
        pointer->stitch.x *= scale;
        pointer->stitch.y *= scale;
        pointer = pointer->next;
    }
}

/*! Returns an EmbRect that encapsulates all stitches and objects in the pattern (\a p). */
EmbRect embPattern_calcBoundingBox(EmbPattern* p)
{
    EmbStitchList* pointer = 0;
    EmbRect boundingRect;
    EmbStitch pt;
    EmbBezier bezier;

    boundingRect.left = 0;
    boundingRect.right = 0;
    boundingRect.top = 0;
    boundingRect.bottom = 0;

    if(!p) { embLog_error("emb-pattern.c embPattern_calcBoundingBox(), p argument is null\n"); return boundingRect; }

    /* Calculate the bounding rectangle.  It's needed for smart repainting. */
    /* TODO: Come back and optimize this mess so that after going thru all objects
            and stitches, if the rectangle isn't reasonable, then return a default rect */
    if (embStitchList_empty(p->stitchList) &&
        !(p->arcs || p->circles || p->ellipses || p->lines || p->points ||
        p->polygons || p->polylines || p->rects || p->splines))
    {
        boundingRect.top = 0.0;
        boundingRect.left = 0.0;
        boundingRect.bottom = 1.0;
        boundingRect.right = 1.0;
        return boundingRect;
    }
    boundingRect.left = 99999.0;
    boundingRect.top =  99999.0;
    boundingRect.right = -99999.0;
    boundingRect.bottom = -99999.0;

    pointer = p->stitchList;
    while(pointer)
    {
        /* If the point lies outside of the accumulated bounding
        * rectangle, then inflate the bounding rect to include it. */
        pt = pointer->stitch;
        if(!(pt.flags & TRIM))
        {
            boundingRect.left = embMinDouble(boundingRect.left, pt.x);
            boundingRect.top = embMinDouble(boundingRect.top, pt.y);
            boundingRect.right = embMaxDouble(boundingRect.right, pt.x);
            boundingRect.bottom = embMaxDouble(boundingRect.bottom, pt.y);
        }
        pointer = pointer->next;
    }

    int i, j;
    if (p->arcs) {
        /* TODO: embPattern_calcBoundingBox for arcs, for now just checks the start point */
        for (i=0; i<p->arcs->count; i++) {
            EmbArc arc = p->arcs->arc[i].arc;
            boundingRect.left = embMinDouble(boundingRect.left, arc.startX);
            boundingRect.top = embMinDouble(boundingRect.top, arc.startY);
            boundingRect.right = embMaxDouble(boundingRect.right, arc.startX);
            boundingRect.bottom = embMaxDouble(boundingRect.bottom, arc.startY);
        }
    }

    if (p->circles) {
        for (i=0; i<p->circles->count; i++) {
            EmbCircle circle = p->circles->circle[i].circle;
            boundingRect.left = embMinDouble(boundingRect.left, circle.centerX - circle.radius);
            boundingRect.top = embMinDouble(boundingRect.top, circle.centerY - circle.radius);
            boundingRect.right = embMaxDouble(boundingRect.right, circle.centerX + circle.radius);
            boundingRect.bottom = embMaxDouble(boundingRect.bottom, circle.centerY + circle.radius);
        }
    }

    if (p->ellipses) {
        for (i=0; i<p->ellipses->count; i++) {
            /* TODO: account for rotation */
            EmbEllipse ellipse = p->ellipses->ellipse[i].ellipse;
            boundingRect.left = embMinDouble(boundingRect.left, ellipse.centerX - ellipse.radiusX);
            boundingRect.top = embMinDouble(boundingRect.top, ellipse.centerY - ellipse.radiusY);
            boundingRect.right = embMaxDouble(boundingRect.right, ellipse.centerX + ellipse.radiusX);
            boundingRect.bottom = embMaxDouble(boundingRect.bottom, ellipse.centerY + ellipse.radiusY);
        }
    }

    if (p->lines) {
        for (i=0; i<p->lines->count; i++) {
            EmbLine line = p->lines->line[i].line;
            boundingRect.left = embMinDouble(boundingRect.left, line.x1);
            boundingRect.left = embMinDouble(boundingRect.left, line.x2);
            boundingRect.top = embMinDouble(boundingRect.top, line.y1);
            boundingRect.top = embMinDouble(boundingRect.top, line.y2);
            boundingRect.right = embMaxDouble(boundingRect.right, line.x1);
            boundingRect.right = embMaxDouble(boundingRect.right, line.x2);
            boundingRect.bottom = embMaxDouble(boundingRect.bottom, line.y1);
            boundingRect.bottom = embMaxDouble(boundingRect.bottom, line.y2);
        }
    }

    if (p->points) {
        for (i=0; i<p->points->count; i++) {
            EmbPoint point = p->points->point[i].point;
            /* TODO: embPattern_calcBoundingBox for points */

        }
    }

    if (p->polygons) {
        for (i=0; i<p->polygons->count; i++) {
            EmbArray *polygon;
            polygon = p->polygons->polygon[i]->pointList;
            for (j=0; j<polygon->count; j++) {
                /* TODO: embPattern_calcBoundingBox for polygons */
            }
        }
    }

    if (p->polylines) {
        for (i=0; i<p->polylines->count; i++) {
            EmbArray *polyline;
            polyline = p->polylines->polyline[i]->pointList;
            for (j=0; j<polyline->count; j++) {
                /* TODO: embPattern_calcBoundingBox for polylines */
            }
        }
    }

    if (p->rects) {
        for (i=0; i<p->rects->count; i++) {
            EmbRect rect = p->rects->rect[i].rect;
            /* TODO: embPattern_calcBoundingBox for rectangles */
        }
    }

    if (p->splines) {
        for (i=0; i<p->splines->count; i++) {
            bezier = p->splines->spline[i].bezier;
            /* TODO: embPattern_calcBoundingBox for splines */

        }
    }

    return boundingRect;
}

/*! Flips the entire pattern (\a p) horizontally about the y-axis. */
void embPattern_flipHorizontal(EmbPattern* p)
{
    if(!p) { embLog_error("emb-pattern.c embPattern_flipHorizontal(), p argument is null\n"); return; }
    embPattern_flip(p, 1, 0);
}

/*! Flips the entire pattern (\a p) vertically about the x-axis. */
void embPattern_flipVertical(EmbPattern* p)
{
    if(!p) { embLog_error("emb-pattern.c embPattern_flipVertical(), p argument is null\n"); return; }
    embPattern_flip(p, 0, 1);
}

/*! Flips the entire pattern (\a p) horizontally about the x-axis if (\a horz) is true.
 *  Flips the entire pattern (\a p) vertically about the y-axis if (\a vert) is true. */
void embPattern_flip(EmbPattern* p, int horz, int vert)
{
    int i, j;
    EmbStitchList* stList = 0;

    if(!p) { embLog_error("emb-pattern.c embPattern_flip(), p argument is null\n"); return; }

    stList = p->stitchList;
    while(stList)
    {
        if(horz) { stList->stitch.x = -stList->stitch.x; }
        if(vert) { stList->stitch.y = -stList->stitch.y; }
        stList = stList->next;
    }

    if (p->arcs) {
        for (i=0; i<p->arcs->count; i++) {
            if (horz) {
                p->arcs->arc[i].arc.startX *= -1.0;
                p->arcs->arc[i].arc.midX *= -1.0;
                p->arcs->arc[i].arc.endX *= -1.0;
            }
            if (vert) {
                p->arcs->arc[i].arc.startY *= -1.0;
                p->arcs->arc[i].arc.midY *= -1.0;
                p->arcs->arc[i].arc.endY *= -1.0;
            }
        }
    }

    if (p->circles) {
        for (i=0; i<p->circles->count; i++) {
            if (horz) { p->circles->circle[i].circle.centerX *= -1.0; }
            if (vert) { p->circles->circle[i].circle.centerY *= -1.0; }
        }
    }

    if (p->ellipses) {
        for (i=0; i<p->ellipses->count; i++) {
            if (horz) { p->ellipses->ellipse[i].ellipse.centerX *= -1.0; }
            if (vert) { p->ellipses->ellipse[i].ellipse.centerY *= -1.0; }
        }
    }

    if (p->lines) {
        for (i=0; i<p->lines->count; i++) {
            if(horz) {
                p->lines->line[i].line.x1 *= -1.0;
                p->lines->line[i].line.x2 *= -1.0;
            }
            if (vert) {
                p->lines->line[i].line.y1 *= -1.0;
                p->lines->line[i].line.y2 *= -1.0;
            }
        }
    }

    if (p->paths) {
        for (i=0; i<p->paths->count; i++) {
            EmbArray *path = p->paths->path[i]->pointList;
            for (j=0; j<path->count; j++) {
                if (horz) {
                    path->point[j].point.x *= -1.0;
                }
                if (vert) {
                    path->point[j].point.y *= -1.0;
                }
            }
        }
    }

    if (p->points) {
        for (i=0; i<p->points->count; i++) {
            if (horz) {
                p->points->point[i].point.x *= -1.0;
            }
            if (vert) {
                p->points->point[i].point.y *= -1.0;
            }
        }
    }

    if (p->polygons) {
        for (i=0; i<p->polygons->count; i++) {
            EmbArray *polygon;
            polygon = p->polygons->polygon[i]->pointList;
            for (j=0; j<polygon->count; j++) {
                if (horz) { polygon->point[j].point.x *= -1.0; }
                if (vert) { polygon->point[j].point.y *= -1.0; }
            }
        }
    }

    if (p->polylines) {
        for (i=0; i<p->polylines->count; i++) {
            EmbArray *polyline;
            polyline = p->polylines->polygon[i]->pointList;
            for (j=0; j<polyline->count; j++) {
                if (horz) { polyline->point[j].point.x *= -1.0; }
                if (vert) { polyline->point[j].point.y *= -1.0; }
            }
        }
    }

    if (p->rects) {
        for (i=0; i<p->rects->count; i++) {
            if (horz) {
                p->rects->rect[i].rect.left *= -1.0;
                p->rects->rect[i].rect.right *= -1.0;
            }
            if (vert) {
                p->rects->rect[i].rect.top *= -1.0;
                p->rects->rect[i].rect.bottom *= -1.0;
            }
        }
    }
    
    if (p->splines) {
        for (i=0; i<p->splines->count; i++) {
            /* TODO: embPattern_flip for splines */
        }
    }
}

void embPattern_combineJumpStitches(EmbPattern* p)
{
    EmbStitchList* pointer = 0;
    int jumpCount = 0;
    EmbStitchList* jumpListStart = 0;

    if(!p) { embLog_error("emb-pattern.c embPattern_combineJumpStitches(), p argument is null\n"); return; }
    pointer = p->stitchList;
    while(pointer)
    {
        if(pointer->stitch.flags & JUMP)
        {
            if(jumpCount == 0)
            {
                jumpListStart = pointer;
            }
            jumpCount++;
        }
        else
        {
            if(jumpCount > 0)
            {
                EmbStitchList* removePointer = jumpListStart->next;
                jumpListStart->stitch.x = pointer->stitch.x;
                jumpListStart->stitch.y = pointer->stitch.y;
                jumpListStart->next = pointer;

                for(; jumpCount > 0; jumpCount--)
                {
                    EmbStitchList* tempPointer = removePointer->next;
                    free(removePointer);
                    removePointer = tempPointer;
                }
                jumpCount = 0;
            }
        }
        pointer = pointer->next;
    }
}

/*TODO: The params determine the max XY movement rather than the length. They need renamed or clarified further. */
void embPattern_correctForMaxStitchLength(EmbPattern* p, double maxStitchLength, double maxJumpLength)
{
    int j = 0, splits;
    double maxXY, maxLen, addX, addY;

    if(!p) { embLog_error("emb-pattern.c embPattern_correctForMaxStitchLength(), p argument is null\n"); return; }
    if(embStitchList_count(p->stitchList) > 1)
    {
        EmbStitchList* pointer = 0;
        EmbStitchList* prev = 0;
        prev = p->stitchList;
        pointer = prev->next;

        while(pointer)
        {
            double xx = prev->stitch.x;
            double yy = prev->stitch.y;
            double dx = pointer->stitch.x - xx;
            double dy = pointer->stitch.y - yy;
            if((fabs(dx) > maxStitchLength) || (fabs(dy) > maxStitchLength))
            {
                maxXY = embMaxDouble(fabs(dx), fabs(dy));
                if(pointer->stitch.flags & (JUMP | TRIM)) maxLen = maxJumpLength;
                else maxLen = maxStitchLength;

                splits = (int)ceil((double)maxXY / maxLen);

                if(splits > 1)
                {
                    int flagsToUse = pointer->stitch.flags;
                    int colorToUse = pointer->stitch.color;
                    addX = (double)dx / splits;
                    addY = (double)dy / splits;

                    for(j = 1; j < splits; j++)
                    {
                        EmbStitchList* item = 0;
                        EmbStitch s;
                        s.x = xx + addX * j;
                        s.y = yy + addY * j;
                        s.flags = flagsToUse;
                        s.color = colorToUse;
                        item = (EmbStitchList*)malloc(sizeof(EmbStitchList));
                        if(!item) { embLog_error("emb-pattern.c embPattern_correctForMaxStitchLength(), cannot allocate memory for item\n"); return; }
                        item->stitch = s;
                        item->next = pointer;
                        prev->next = item;
                        prev = item;
                    }
                }
            }
            prev = pointer;
            if(pointer)
            {
                pointer = pointer->next;
            }
        }
    }
    if(p->lastStitch && p->lastStitch->stitch.flags != END)
    {
        embPattern_addStitchAbs(p, p->lastStitch->stitch.x, p->lastStitch->stitch.y, END, 1);
    }
}

void embPattern_center(EmbPattern* p)
{
    /* TODO: review this. currently not used in anywhere. Also needs to handle various design objects */
    int moveLeft, moveTop;
    EmbRect boundingRect;
    EmbStitchList* pointer = 0;

    if(!p) { embLog_error("emb-pattern.c embPattern_center(), p argument is null\n"); return; }
    boundingRect = embPattern_calcBoundingBox(p);

    moveLeft = (int)(boundingRect.left - (embRect_width(boundingRect) / 2.0));
    moveTop = (int)(boundingRect.top - (embRect_height(boundingRect) / 2.0));

    pointer = p->stitchList;
    while(pointer)
    {
        EmbStitch s;
        s = pointer->stitch;
        s.x -= moveLeft;
        s.y -= moveTop;
    }
}

/*TODO: Description needed. */
void embPattern_loadExternalColorFile(EmbPattern* p, const char* fileName)
{
#ifdef ARDUINO
    return; /* TODO ARDUINO: This function leaks memory. While it isn't crucial to running the machine, it would be nice use this function, so fix it up. */
#endif /* ARDUINO */

    char hasRead = 0;
    EmbReaderWriter* colorFile = 0;
    const char* dotPos = strrchr(fileName, '.');
    char* extractName = 0;

    if(!p) { embLog_error("emb-pattern.c embPattern_loadExternalColorFile(), p argument is null\n"); return; }
    if(!fileName) { embLog_error("emb-pattern.c embPattern_loadExternalColorFile(), fileName argument is null\n"); return; }

    extractName = (char*)malloc(dotPos - fileName + 5);
    if(!extractName) { embLog_error("emb-pattern.c embPattern_loadExternalColorFile(), cannot allocate memory for extractName\n"); return; }
    extractName = (char*)memcpy(extractName, fileName, dotPos - fileName);
    extractName[dotPos - fileName] = '\0';
    strcat(extractName,".edr");
    colorFile = embReaderWriter_getByFileName(extractName);
    if(colorFile)
    {
        hasRead = (char)colorFile->reader(p, extractName);
    }
    if(!hasRead)
    {
        free(colorFile);
        colorFile = 0;
        extractName = (char*)memcpy(extractName, fileName, dotPos - fileName);
        extractName[dotPos - fileName] = '\0';
        strcat(extractName,".rgb");
        colorFile = embReaderWriter_getByFileName(extractName);
        if(colorFile)
        {
            hasRead = (char)colorFile->reader(p, extractName);
        }
    }
    if(!hasRead)
    {
        free(colorFile);
        colorFile = 0;
        extractName = (char*)memcpy(extractName, fileName, dotPos - fileName);
        extractName[dotPos - fileName] = '\0';
        strcat(extractName,".col");
        colorFile = embReaderWriter_getByFileName(extractName);
        if(colorFile)
        {
            hasRead = (char)colorFile->reader(p, extractName);
        }
    }
    if(!hasRead)
    {
        free(colorFile);
        colorFile = 0;
        extractName = (char*)memcpy(extractName, fileName, dotPos - fileName);
        extractName[dotPos - fileName] = '\0';
        strcat(extractName,".inf");
        colorFile = embReaderWriter_getByFileName(extractName);
        if(colorFile)
        {
            hasRead = (char)colorFile->reader(p, extractName);
        }
    }
    free(colorFile);
    colorFile = 0;
    free(extractName);
    extractName = 0;
}

/*! Frees all memory allocated in the pattern (\a p). */
void embPattern_free(EmbPattern* p)
{
    if (!p) {
        embLog_error("emb-pattern.c embPattern_free(), p argument is null\n");
        return;
    }
    embStitchList_free(p->stitchList);
    p->stitchList = 0;
    p->lastStitch = 0;

    embArray_free(p->threads);
    embArray_free(p->arcs);
    embArray_free(p->circles);
    embArray_free(p->ellipses);
    embArray_free(p->lines);
    embArray_free(p->paths);
    embArray_free(p->points);
    embArray_free(p->polygons);
    embArray_free(p->polylines);
    embArray_free(p->rects);
    embArray_free(p->splines);

    free(p);
    p = 0;
}

/*! Adds a circle object to pattern (\a p) with its center at the absolute
 * position (\a cx,\a cy) with a radius of (\a r). Positive y is up.
 * Units are in millimeters. */
void embPattern_addCircleObjectAbs(EmbPattern* p, double cx, double cy, double r)
{
    EmbCircle circle = {cx, cy, r};

    if (!p) { embLog_error("emb-pattern.c embPattern_addCircleObjectAbs(), p argument is null\n"); return; }
    if (p->circles == 0) {
         p->circles = embArray_create(EMB_CIRCLE);
    }
    embArray_addCircle(p->circles, circle, 0, black);
}

/*! Adds an ellipse object to pattern (\a p) with its center at the
 * absolute position (\a cx,\a cy) with radii of (\a rx,\a ry). Positive y is up.
 * Units are in millimeters. */
void embPattern_addEllipseObjectAbs(EmbPattern* p, double cx, double cy, double rx, double ry)
{
    EmbEllipse ellipse;
    ellipse.centerX = cx;
    ellipse.centerY = cy;
    ellipse.radiusX = rx;
    ellipse.radiusY = ry;

    if (!p) {
        embLog_error("emb-pattern.c embPattern_addEllipseObjectAbs(), p argument is null\n");
        return;
    }
    if (!p->ellipses) {
        p->ellipses = embArray_create(EMB_ELLIPSE);
    }
    embArray_addEllipse(p->ellipses, ellipse, 0.0, 0, black);
}

/*! Adds a line object to pattern (\a p) starting at the absolute position
 * (\a x1,\a y1) and ending at the absolute position (\a x2,\a y2).
 * Positive y is up. Units are in millimeters.
 */
void embPattern_addLineObjectAbs(EmbPattern* p, double x1, double y1, double x2, double y2)
{
    EmbLineObject lineObj;
    lineObj.line = embLine_make(x1, y1, x2, y2);
    lineObj.color = embColor_make(0, 0, 0);

    if (!p) {
        embLog_error("emb-pattern.c embPattern_addLineObjectAbs(), p argument is null\n");
        return;
    }
    if (p->circles == 0) {
         p->lines = embArray_create(EMB_LINE);
    }
    embArray_addLine(p->lines, lineObj);
}

void embPattern_addPathObjectAbs(EmbPattern* p, EmbPathObject* obj)
{
    if(!p) { embLog_error("emb-pattern.c embPattern_addPathObjectAbs(), p argument is null\n"); return; }
    if(!obj) { embLog_error("emb-pattern.c embPattern_addPathObjectAbs(), obj argument is null\n"); return; }
    if(!obj->pointList) { embLog_error("emb-pattern.c embPattern_addPathObjectAbs(), obj->pointList is empty\n"); return; }

    if (!p->paths) {
        p->paths = embArray_create(EMB_PATH);
    }
    embArray_addPath(p->paths, obj);
}

/*! Adds a point object to pattern (\a p) at the absolute position (\a x,\a y). Positive y is up. Units are in millimeters. */
void embPattern_addPointObjectAbs(EmbPattern* p, double x, double y)
{
    EmbPointObject pointObj;
    pointObj.point.x = x;
    pointObj.point.y = y;

    if (!p) {
        embLog_error("emb-pattern.c embPattern_addPointObjectAbs(), p argument is null\n");
        return;
    }
    if (!p->points) {
        p->points = embArray_create(EMB_POINT);
    }
    embArray_addPoint(p->points, &pointObj);
}

void embPattern_addPolygonObjectAbs(EmbPattern* p, EmbPolygonObject* obj)
{
    if(!p) { embLog_error("emb-pattern.c embPattern_addPolygonObjectAbs(), p argument is null\n"); return; }
    if(!obj) { embLog_error("emb-pattern.c embPattern_addPolygonObjectAbs(), obj argument is null\n"); return; }
    if(!obj->pointList) { embLog_error("emb-pattern.c embPattern_addPolygonObjectAbs(), obj->pointList is empty\n"); return; }

    if (!p->polygons) {
        p->polygons = embArray_create(EMB_POLYGON);
    }
    embArray_addPolygon(p->polygons, obj);
}

void embPattern_addPolylineObjectAbs(EmbPattern* p, EmbPolylineObject* obj)
{
    if (!p) { embLog_error("emb-pattern.c embPattern_addPolylineObjectAbs(), p argument is null\n"); return; }
    if (!obj) { embLog_error("emb-pattern.c embPattern_addPolylineObjectAbs(), obj argument is null\n"); return; }
    if (!obj->pointList) {
        embLog_error("emb-pattern.c embPattern_addPolylineObjectAbs(), obj->pointList is empty\n"); return;
    }

    if (!p->polylines) {
        p->polylines = embArray_create(EMB_POLYLINE);
    }
    embArray_addPolyline(p->polylines, obj);
}

/**
 * Adds a rectangle object to pattern (\a p) at the absolute position
 * (\a x,\a y) with a width of (\a w) and a height of (\a h).
 * Positive y is up. Units are in millimeters.
 */
void embPattern_addRectObjectAbs(EmbPattern* p, double x, double y, double w, double h)
{
    EmbRect rect;
    rect.left = x;
    rect.top = y;
    rect.right = x+w;
    rect.bottom = y+h;

    if (!p) {
        embLog_error("emb-pattern.c embPattern_addRectObjectAbs(), p argument is null\n");
        return;
    }
    if (!p->rects) {
        p->rects = embArray_create(EMB_RECT);
    }
    embArray_addRect(p->rects, rect, 0, black);
}

void embPattern_end(EmbPattern *p)
{
    /* Check for an END stitch and add one if it is not present */
    if (p->lastStitch->stitch.flags != END) {
        embPattern_addStitchRel(p, 0, 0, END, 1);
    }
}


