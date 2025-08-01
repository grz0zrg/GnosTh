#ifdef d
#!/bin/sh
set -e

rm -f *.o *.elf *.bin
arm-linux-gnueabihf-as --warn --fatal-warnings start.s -o start.o
#clang --target=arm-linux-gnueabihf -march=armv6 -mtune=arm1176jzf-s -mfloat-abi=soft -c start.s -o start.o
#clang -Wall -O3 --target=arm-linux-gnueabihf -march=armv6 -mtune=arm1176jzf-s -mfloat-abi=soft -ffreestanding -nostdlib -c logo.c -o program.o
arm-linux-gnueabihf-gcc -Wall -O3 -march=armv6 -mtune=arm1176jzf-s -mfloat-abi=soft -ffreestanding -nostdlib -c logo.c -o program.o
arm-linux-gnueabihf-ld -T rpi0.ld -nostdlib start.o program.o -o program.elf
arm-linux-gnueabihf-objcopy program.elf -O binary program.bin
arm-linux-gnueabihf-objdump -D -b binary -m arm program.bin --adjust-vma=0x80000
rm *.o *.elf
wc -c program.bin
exit
#endif

#define uint unsigned int

#define FB_BASE 0x1e99a000
#define FB_SIZE 0x95ffc
#define FB_WIDTH 0x400
#define FB_HEIGHT 0x258
#define FB_BPP 4
#define FB_BPP_SHIFT 2

volatile uint* fb_ptr = (volatile uint*)FB_BASE;

uint rseed = 0;
void random_seed(uint seed) {
    rseed = seed;
}

uint random16() {
    rseed = 0xbb75 * rseed & 0xffff;
    return rseed;
}

int is_in_bounds(int n, int l, int h) {
    return (n > l) ? (n < h) ? 1 : 0 : 0;
}

int clamp8(int n) {
    if (n < 0) return 0;
    if (n > 0xff) return 0xff;
    return n;
}

uint rgb_pack32(int r, int g, int b) {
    return (b << 0x10) | (g << 8) | r;
}

uint color_pack32(int c) {
    return rgb_pack32(c, c, c);
}

void plot(uint i, uint c) {
    fb_ptr[i] = c;
}

void plot_xy(int x, int y, uint c) {
    plot(x + y * FB_WIDTH, c);
}

void cls(uint color) {
    for (int i = FB_SIZE; i >= 0; i--) {
        fb_ptr[i] = color;
    }
}

uint ascii_map_3x3[] = {
	0,0x89,0x2d,0x145,0xfe,0x1ff,0x1eb,0x12,0x19e,0xf3,0x155,0xba,0x52,0x38,0x10,0x54,
	0x1ef,0x1d3,0x193,0x1f7,0x13d,0xd6,0x1f9,0x127,0x1fe,0x13f,
	0x82,0x52,0x18e,0x1c7,0xe3,0x77,0x1aa,
	0x17a,0x1fb,0x1cf,0xeb,0x1df,0x5f,0x1eb,0x17d,0x1d7,0x1ec,0x15d,0x1c9,0x17f,0x16f,0x1ef,0x7f,0x13f,0x4f,0xd6,0x97,0x1ed,0xad,0x1fd,0x155,0x95,0x193,
	0x1cf,0x111,0x1e7,0x2a,0x1c0
};

uint ascii_to_glyph(uint n) {
    return ascii_map_3x3[n];
}

void draw_3x3d_glyph(uint id, uint ox, uint oy, uint scale) {
    uint glyph_size = (3 << scale) - 1;
    for (uint x = 0; x < glyph_size; x++) {
        uint gx = x >> scale;
        for (uint y = 0; y < glyph_size; y += 5) {
            uint gi = gx + (y >> scale) * 3;
            if ((id & (1 << gi)) > 0) {
                for (uint z = 0; z < 0x5c; z += 4) {
                    uint color = 0x00ffffff;
                    uint i1 = z >> 1;
                    if (z > 0) // depth shading
                        color = color_pack32(clamp8(0x3f - (i1 >> 1) * 3));
                    if (y > (glyph_size - 3)) color = 0x00080808; // bottom shading

                    uint fx = (ox + x) + i1;
                    uint fy = (oy + y) - (x >> 2) + i1;
                    plot_xy(fx, fy + 0, color);
                    plot_xy(fx, fy + 1, color);
                    plot_xy(fx, fy + 2, color);
                    plot_xy(fx, fy + 3, color);
                }
            }
        }
    }
}

void draw_back_lines() {
    int mx = 0, my = 0x1ff;
    for (uint y = 0x30; y < 0x227; y += 0x18) {
        uint ox = (random16() >> 8) + 0x18;
        for (int i = ox; i < (FB_WIDTH - ox); i++) {
            mx += (my >> 7);
            my -= (mx >> 5);

            int r = clamp8(0x80 - (mx >> 1));
            int g = clamp8(0x5c - (mx >> 1));
            int b = clamp8((0xff - mx) >> 1);
            uint c = rgb_pack32(r, g, b);

            plot_xy(i, y + ((mx + my) >> 7), c);
        }
    }
}

void draw_back_stars() {
    for (int i = 0x1ff; i >= 0; i--) {
        uint rn = random16();
        uint index = rn * 9;

        if (is_in_bounds(index, 0x6000, 0x90000)) {
            uint y = index % FB_WIDTH;
            if (is_in_bounds(y, 0x18, 0x3e8))
                plot(index, color_pack32(rn & 0xff));
        }
    }
}

void main() {
    random_seed(0xd);
    cls(0);

    draw_back_lines(); draw_back_stars();
    draw_3x3d_glyph(ascii_to_glyph(0x2f), 0xa6, 0xcc, 6);
    draw_3x3d_glyph(ascii_to_glyph(0x32),0x1a0, 0xcc, 6);
    draw_3x3d_glyph(ascii_to_glyph(0x3a),0x29a, 0xcc, 6);
}
