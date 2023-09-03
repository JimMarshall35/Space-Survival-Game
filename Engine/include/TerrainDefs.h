#define BASE_CELL_SIZE 16 // the size of all cells. lowest mip level is sampled 16x16x16 in actual dimensions with a cell size of 2, next level is 32x32x32 ect with a cell size of 4, ect. 
#define POLYGONIZER_NEGATIVE_GUTTER 1
#define POLYGONIZER_POSITIVE_GUTTER 2
#define TOTAL_SIDE_SIZE (BASE_CELL_SIZE + POLYGONIZER_NEGATIVE_GUTTER + POLYGONIZER_POSITIVE_GUTTER)
#define TOTAL_CELL_VOLUME_SIZE (TOTAL_SIDE_SIZE*TOTAL_SIDE_SIZE*TOTAL_SIDE_SIZE)