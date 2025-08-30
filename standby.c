#include <raylib.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <math.h>

typedef uint8_t byte;

#define TEMP_BUFFER_CAP (4*1024)
static_assert(TEMP_BUFFER_CAP%8==0);

_Thread_local size_t temp_allocated = 0;
_Thread_local byte temp_buffer[TEMP_BUFFER_CAP];

size_t align(size_t n) {
	if (n%8 == 0) return n;
	return n + (8-n%8);
}

void* talloc(size_t n) {
	size_t size = align(n);
	assert(size <= TEMP_BUFFER_CAP);

	if (temp_allocated + size >= TEMP_BUFFER_CAP) {
		temp_allocated = 0;
	}

	void* ptr = &temp_buffer[temp_allocated];
	temp_allocated += size;
	memset(ptr, 0, size);
	return ptr;
}

void treset() {
	temp_allocated = 0;
}

size_t tmark(void) { return temp_allocated; }
void trelease(size_t mark) { temp_allocated = mark; }

typedef struct {
	size_t count;
	char* data;
} String;

String tprintf(const char* format, ...) {
	va_list args, args2;
	va_start(args, format);
	va_copy(args2, args);

	int n = vsnprintf(NULL, 0, format, args);
	assert(n > 0);

	size_t size = n+1;
	char* buffer = talloc(size);
	if(vsnprintf(buffer, size, format, args2) < 0) {
		va_end(args);
		va_end(args2);
		return (String){0};
	}

	va_end(args);
	va_end(args2);
	return (String){n, buffer};
}

float lerp(float a, float b, float t) {
	return a + (b-a) * t;
}

float normalize(float min, float max, float x) {
	return (x - min)/(max-min);
}

typedef struct tm LocalTime;

LocalTime* get_localtime() {
	time_t now;
	time(&now);
	return localtime(&now);
}

typedef struct {
	bool visible;
	float x;
	float y;
} CelestialPos;

CelestialPos get_moon_pos(int hour) {
	// moon is hidden
	if (hour > 5 && hour < 19) return (CelestialPos){0};

	// moon is visible
	float t;
	if (hour >= 20 && hour < 24) {
		t = normalize(19, 24, hour)*0.5;
	} else {
		t = normalize(0, 5, hour)*0.5+0.5;
	}

	float tx = -t+1;
	float ty = pow(1-t*t, 2);

	return (CelestialPos){.visible=true, .x=tx*GetScreenWidth(), .y=ty*GetScreenHeight()};
}

float get_moon_radius(int hour) {
	// moon is hidden
	if (hour > 5 && hour < 19) return 0;

	// moon is visible
	float t;
	if (hour >= 20 && hour < 24) {
		t = normalize(19, 24, hour)*0.5;
	} else {
		t = normalize(0, 5, hour)*0.5+0.5;
	}

	return lerp(GetScreenHeight()/12, GetScreenHeight()/6, t);
}

CelestialPos get_sun_pos(int hour) {
	// sun is visible
	if (hour >= 5 && hour <= 19) {
		float t = normalize(5, 19, hour);

		float tx = t;
		float ty = t*t;

		return (CelestialPos){.visible=true, .x=tx*GetScreenWidth(), .y=ty*GetScreenHeight()};
	}

	// sun is hidden
	return (CelestialPos){0};
}

float get_sun_radius(int hour) {
	// sun is visible
	if (hour > 5 && hour < 19) {
		float t = normalize(5, 19, hour);
		return lerp(GetScreenWidth()/8, GetScreenHeight()/5, t);
	}

	// sun is hidden
	return 0;
}

Color sun_color(int hour) {
	// sun is visible
	if (hour > 5 && hour < 19) {
		float t = normalize(5, 19, hour);
		return ColorLerp(RED, ORANGE, t);
	}

	// sun is hidden
	return (Color){0};
}

Color moon_color(int hour) {
	// moon is hidden
	if (hour > 5 && hour < 19) return (Color){0};

	// moon is visible
	float t;
	if (hour >= 20 && hour < 24) {
		t = normalize(19, 24, hour)*0.5;
	} else {
		t = normalize(0, 5, hour)*0.5+0.5;
	}

	return ColorLerp(BLACK, WHITE, t);
}


Color sky_color(float hour) {
    if (hour >= 4 && hour < 6) {
        float t = normalize(4, 6, hour);
        return ColorLerp(BLACK, DARKBROWN, t);
    } else if (hour >= 6 && hour < 15) {
        float t = normalize(6, 15, hour);
        return ColorLerp(DARKBROWN, SKYBLUE, t);
    } else if (hour >= 15 && hour < 20) {
        float t = normalize(15, 20, hour);
        return ColorLerp(SKYBLUE, DARKBLUE, t);
    } else if (hour >= 20 && hour < 24) {
        float t = normalize(20, 24, hour);
        return ColorLerp(DARKBLUE, BLACK, t);
    }

    return BLACK;
}

int main(int, char**) {
	SetConfigFlags(FLAG_WINDOW_TOPMOST | FLAG_FULLSCREEN_MODE);
	SetTargetFPS(30);
	InitWindow(GetScreenWidth(), GetScreenHeight(), "Standby");

	int TITLE_SIZE = GetScreenHeight()/4;
	// int SUBTITLE_SIZE = GetScrenHeight()/10;

	// float hour = 0;
	// float minute = 0;
	while(!WindowShouldClose()) {
		LocalTime* t = get_localtime();
		int hour = t->tm_hour;
		int minute = t->tm_min;

		BeginDrawing();
		ClearBackground(sky_color(hour));

		CelestialPos moon_pos = get_moon_pos(hour);
		if (moon_pos.visible) DrawCircle(moon_pos.x, moon_pos.y, get_moon_radius(hour), moon_color(hour));

		CelestialPos sun_pos = get_sun_pos(hour);
		if (sun_pos.visible) DrawCircle(sun_pos.x, sun_pos.y, get_sun_radius(hour), sun_color(hour));

		String title = tprintf("%d:%d",(int)hour,(int)minute);
		int w = MeasureText(title.data, TITLE_SIZE);

		DrawText(title.data, GetScreenWidth()/2-w/2, GetScreenHeight()/3, TITLE_SIZE, WHITE);
		EndDrawing();

		// float speed = 1.0f;
		// hour += GetFrameTime() * speed;
		// hour = fmodf(hour, 24.0f);
	}

	CloseWindow();
	return 0;
}
