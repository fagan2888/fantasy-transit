#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>

#include <vector>
#include <map>
#include <algorithm>

struct loc {
	int a;
	int o;

	bool operator<(const loc &other) const {
		if (a < other.a) {
			return true;
		}
		if (a == other.a && o < other.o) {
			return true;
		}

		return false;
	}
};

struct latlon {
	float lat;
	float lon;
	double count;

	latlon(double _lat, double _lon, double _count) {
		lat = _lat;
		lon = _lon;
		count = _count;
	}
};

#define FOOT 0.00000274
#define BUCKET (2640 * FOOT)
#define RADIUS (2640 * FOOT)

#define BUCKET2 (5280 * 3 * FOOT)

double fpow(double b, double e) {
	if (b == 0) {
		return 0;
	} else if (e == 2) {
		return b * b;
	} else {
		return exp(log(b) * e);
	}
}

double boxcox(double x, double l) {
	if (l == 0) {
		return log(x);
	}

	return (fpow(x, l) - 1) / l;
}

double pdf(double x, double a, double u, double o, double l) {
	return a * (fpow(x, l - 1)) / (boxcox(o, l) * sqrt(2 * M_PI)) * exp(- fpow(boxcox(x, l) - boxcox(u, l), 2) / (2 * fpow(boxcox(o, l), 2)));
}

struct countcmp {
        bool operator()(struct latlon *const &a, struct latlon *const &b) {
                return b->count < a->count;
        }
} countcmp;

double scale(double v) {
	return exp(log(v) * .7);
}

int main() {
	char s[2000];

	std::multimap<loc, latlon *> map;

	FILE *f = fopen("/data/data/tiger/all-corners", "r");
	// FILE *f = fopen("all-corners-37-122", "r");
	while (fgets(s, 2000, f)) {
		double lat, lon;

		if (sscanf(s, "%lf %lf", &lat, &lon) != 2) {
			fprintf(stderr, "??? %s\n", s);
			continue;
		}

		struct loc loc;
		loc.a = lat / BUCKET;
		double rat = cos(loc.a * BUCKET * M_PI / 180);
		loc.o = lon / (BUCKET / rat);

		struct latlon *ll = new latlon(lat, lon, 0);

		map.insert(std::pair<struct loc, struct latlon *>(loc, ll));
	}
	fclose(f);

	int seq = 0;
	int opercent = -1;

	f = fopen("all-employment-2013", "r");
	while (fgets(s, 2000, f)) {
		std::vector<double> fields;

		seq++;
		int percent = seq * 100 / 6632862;
		if (percent != opercent) {
			fprintf(stderr, "%d%%\n", percent);
			opercent = percent;
		}

		char *cp = s;
		while (*cp != '\0') {
			fields.push_back(atof(cp));
			while (*cp != '\0' && *cp != ',') {
				cp++;
			}
			if (*cp == ',') {
				cp++;
			}
		}

		double lat = fields[53];
		double lon = fields[54];

		double rat = cos(lat * M_PI / 180);

		int a = lat / BUCKET;

		int aa, oo;
		for (aa = a - 1; aa <= a + 1; aa++) {
			double brat = cos(aa * BUCKET * M_PI / 180);
			int o = lon / (BUCKET / brat);

			for (oo = o - 1; oo <= o + 1; oo++) {
				struct loc l;
				l.a = aa;
				l.o = oo;

				std::pair<std::multimap<loc, latlon *>::iterator, std::multimap<loc, latlon *>::iterator> range;
				range = map.equal_range(l);

				for (std::multimap<loc, latlon *>::iterator it = range.first; it != range.second; ++it) {
					// printf("for %f,%f found %f,%f\n", lat, lon, it->second->lat, it->second->lon);

					double latd = lat - it->second->lat;
					double lond = (lon - it->second->lon) * rat;

					double dsq = latd * latd + lond * lond;
					if (dsq > RADIUS * RADIUS) {
						continue;
					}

					double dist = sqrt(dsq) / FOOT;

					double workweight = (pdf(dist, 133686, 990, 37.4624, 0.312) +
						             pdf(dist, 37600, 6376, 1.00334, -0.588)) /
						      (10 * (pdf(dist, 155673, 4634.18, 7.78672, 0.065896) +
							     pdf(dist, 12108.1, 38096.7, 13.4126, 0.281497)));
					double homeweight = (pdf(dist, 145726, 5859.81, 1.87102, -0.083514)) /
						      (10 * (pdf(dist, 137748, 7757, 3.8884, 0.0558907) +
							     pdf(dist, 13593.1, 39918.2, 27.3165, 0.342803)));

					// workweight *= 0.863528;
					// homeweight *= 1.58911;

					it->second->count += workweight * fields[1] + homeweight * fields[57];
				}
			}
		}
	}

	std::vector<latlon *> possible;
	for (std::multimap<loc, latlon *>::iterator it = map.begin(); it != map.end(); ++it) {
		if (it->second->count > 1250) {
			possible.push_back(it->second);
		}
	}
	std::sort(possible.begin(), possible.end(), countcmp);

	std::multimap<loc, latlon *> seen;
	for (size_t i = 0; i < possible.size(); i++) {
		double closest = INT_MAX;
		double closecount = 0;

		double lat = possible[i]->lat;
		double lon = possible[i]->lon;
		double rat = cos(lat * M_PI / 180);
		int a = lat / BUCKET2;
		int aa, oo;
		for (aa = a - 1; aa <= a + 1; aa++) {
			double brat = cos(aa * BUCKET2 * M_PI / 180);
			int o = lon / (BUCKET2 / brat);

			for (oo = o - 1; oo <= o + 1; oo++) {
				struct loc l;
				l.a = aa;
				l.o = oo;

				std::pair<std::multimap<loc, latlon *>::iterator, std::multimap<loc, latlon *>::iterator> range;
				range = seen.equal_range(l);

				for (std::multimap<loc, latlon *>::iterator it = range.first; it != range.second; ++it) {
					double latd = lat - it->second->lat;
					double lond = (lon - it->second->lon) * rat;

					double dsq = latd * latd + lond * lond;
					if (dsq < BUCKET2 * BUCKET2) {
						double d = sqrt(dsq);
						if (d < closest) {
							closest = d;
							closecount = it->second->count;
						}
					}
				}
			}
		}

		if (closest > BUCKET2) {
			closest = BUCKET2;
		}

		if (closecount == 0) {
			closecount = possible[i]->count;
		} else {
			closecount = sqrt(possible[i]->count * closecount);
		}

		double min = 5280 * FOOT / (scale(closecount) / scale(5000));
		if (min < 1320 * FOOT) {
			min = 1320 * FOOT;
		}
		if (closest > min) {
			struct loc loc;
			loc.a = lat / BUCKET2;
			rat = cos(loc.a * BUCKET2 * M_PI / 180);
			loc.o = lon / (BUCKET2 / rat);

			printf("%f,%f %f\n", possible[i]->lat, possible[i]->lon, possible[i]->count);
			seen.insert(std::pair<struct loc, struct latlon *>(loc, possible[i]));
		}
	}
}
