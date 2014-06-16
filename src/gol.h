#ifndef GOL_H
#define GOL_H

struct gol;

struct gol *
gol_new(int width, int height);

void
gol_update(struct gol *gol);

void
gol_paint(struct gol *gol);

void
gol_free(struct gol *gol);

#endif /* GOL_H */
