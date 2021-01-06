#include <list>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <unistd.h>
#include <ctype.h>
#include <bits/stdc++.h>
#include "regex.h"
#include "regexutils.h"

void showhelp(char * name) {
    std::cout << "Use: " << name << " [OPTIONS] patternfile subjectfile" << std::endl;
    std::cout << std::endl;
    std::cout << "You can pass subject through stdin, just give the '-' as subjectfile or leave it" << std::endl;
    std::cout << std::endl;
    std::cout << "OPTIONS:" << std::endl;
    std::cout << "\t-h\tThis help" << std::endl;
    std::cout << "\t-n N\titerate pcre_regex as Nth times. Default value is 1." << std::endl;
    std::cout << "\t-f\tForce to use old modsec v3 regex matching method." << std::endl;
    std::cout << "\t-t T\tExpects a float value; if the (last) pcre_exec time is greater than this," << std::endl;
    std::cout << "\t    \tthe exit status of program will non-zero." << std::endl;
    std::cout << "\t-d  \tShow detailed information." << std::endl;
    std::cout << std::endl;
}

int main(int argc, char ** argv) {
    Regex *re;
    char rcerror[100];
    char * patternfile = NULL, * subjectfile = NULL;
    char c;
    int icnt = 1, rc = 0;
    bool use_old = false;
    float time_limit = 0.0;
    // double m_sub = 0.0;
    int debuglevel = 0;  // may be later we can use different level...
    char stdinname[] = "-";

    struct timespec ts_before, ts_after, ts_diff;
    std::vector<long double> ld_diffs;
    long double ld_minval, ld_maxval;
    long double ld_sums, ld_mean, ld_med;

    ld_sums         = 0.0;
    ld_med          = 0.0;
    ld_minval       = 0.0;
    ld_maxval       = 0.0;

    if (argc < 2) {
      showhelp(argv[0]);
      return EXIT_FAILURE;
    }

    while ((c = getopt (argc, argv, "hfn:t:d")) != -1) {
        switch (c) {
            case 'h':
                showhelp(argv[0]);
                return EXIT_SUCCESS;
            case 'f':
                use_old = true;
                break;
            case 'n':
                icnt = atoi(optarg);
                if (icnt <= 0 || icnt > INT_MAX) {
                    std::cerr << "Ohh... Try to pass for '-n' an integer between 1 and " << INT_MAX << std::endl;
                    return EXIT_FAILURE;
                }
                break;
            case 't':
                time_limit = atof(optarg);
                if (time_limit <= 0.0) {
                    fprintf(stderr, "Ohh... Time limit should be positive value.\n");
                    return EXIT_FAILURE;
                }
                break;
            case 'd':
                debuglevel = 1;
                break;
            case '?':
                if (optopt == 'n' || optopt == 't') {
                    std::cerr << "Option -" << (char)optopt << " requires an argument." << std::endl;
                }
                else if (isprint (optopt)) {
                    std::cerr << "Unknown option `-" << (char)optopt << "'." << std::endl;
                }
                else {
                    std::cerr << "Unknown option character `\\x%x'." << std::endl;
                }
                return EXIT_FAILURE;
            default:
                abort ();
        }
    }

    for (int i = optind; i < argc; i++) {
        if (patternfile == NULL) {
            patternfile = argv[i];
        }
        else {
            if (subjectfile == NULL) {
                subjectfile = argv[i];
            }
        }
    }

    if (subjectfile == NULL) {
        subjectfile = stdinname;
    }

    if (patternfile == NULL) {
        showhelp(argv[0]);
        return EXIT_FAILURE;
    }

    // read pattern
    std::ifstream pattf(patternfile);
    std::string pattern;
    if (pattf) {
        getline(pattf, pattern);
    }
    else {
        std::cout << "Can't open file: " << patternfile << std::endl;
    }

    debugvalue(debuglevel, std::string("PATTERN"), pattern);

    std::string subject;
    // read subject
    //   if filename was given
    if (strcmp(subjectfile, "-") != 0) {
        std::ifstream subjf(subjectfile);
        if (subjf) {
            subject.assign((std::istreambuf_iterator<char>(subjf)),
                                (std::istreambuf_iterator<char>()));
        }
        else {
            std::cout << "Can't open file: " << subjectfile << std::endl;
        }
    }
    //   or read from stdin
    else {
        std::getline(std::cin, subject);
    }

    debugvalue(debuglevel, std::string("SUBJECT"), subject);

    re = new Regex(pattern, debuglevel);
    std::list<SMatch> retval;
    std::vector<SMatchCapture> captures;

    for(int i = 0; i < icnt; i++) {

        re->m_retList.clear();

        ts_diff.tv_sec  = 0;
        ts_diff.tv_nsec = 0;

        clock_gettime(CLOCK_REALTIME, &ts_before);
        if (use_old == false) {
            captures.clear();
            re->searchOneMatch(subject, captures);
            rc = captures.size();
        }
        else {
            retval = re->searchAll(subject);
            rc = retval.size();
        }
        clock_gettime(CLOCK_REALTIME, &ts_after);
        timespec_diff(&ts_after, &ts_before, &ts_diff);
        // minimal value of re->m_execrc is 0, this means no match
        // in this case we have to decrease the valur for the correct message
        if (rc == 0) {
            rc = -1;
        }
        translate_error(rc, rcerror);
        debugvalue(debuglevel, std::string("RESULT"), std::string(""));
        // std::cout << patternfile << " - time elapsed: " << std::fixed << std::setfill('0') << std::setw(6) << m_sub << ", match value: " << rcerror << std::endl;
        std::cout << patternfile << " - time elapsed: " << ts_diff.tv_sec << "." << std::fixed << std::setfill('0') << std::setw(9) << ts_diff.tv_nsec << ", match value: " << rcerror << std::endl;
        if (icnt > 1) {
            ld_diffs.push_back(ts_diff.tv_sec + (ts_diff.tv_nsec/1000000000.0));

            ld_sums += ld_diffs[i];
            // set minval
            if (i == 0 || ld_diffs[i] < ld_minval) {
                ld_minval = ld_diffs[i];
            }
            // set maxval
            if (ld_diffs[i] > ld_maxval) {
                ld_maxval = ld_diffs[i];
            }
        }
    }

    if (icnt > 1) {
        ld_mean = ld_sums / (double)icnt;

        qsort(&ld_diffs[0], icnt, sizeof(long double), compare_ld);

        if (icnt%2 == 1) {
           ld_med = ld_diffs[(icnt/2)+1];
        }
        else {
            long double ldtemp[2];
            ldtemp[0] = ld_diffs[(icnt/2)];
            ldtemp[1] = ld_diffs[(icnt/2)+1];
            ld_med = ldtemp[0] + ((ldtemp[1]-ldtemp[0])/2.0);
        }

        printf("Num of values: %d\n", icnt);
        printf("         Mean: %013.9Lf\n", ld_mean);
        printf("       Median: %013.9Lf\n", ld_med);
        printf("          Min: %013.9Lf\n", ld_minval);
        printf("          Max: %013.9Lf\n", ld_maxval);
        printf("        Range: %013.9Lf\n", ld_maxval - ld_minval);
        printf("Std deviation: %013.9Lf\n", calc_std_deviation(&ld_diffs[0], icnt, ld_mean));
    }

    // show captured substrings if debug was set
    if (debuglevel == 1) {
        debugvalue(debuglevel, "CAPTURES", "");
        if (use_old == false) {
            for (const SMatchCapture& capture : captures) {
                const std::string capture_substring(subject.substr(capture.m_offset, capture.m_length));
                std::string subpatt = "";
                if (capture.m_offset > 0) {
                    subpatt += subject.substr(0, capture.m_offset);
                }
                subpatt += BOLDGREEN + capture_substring + RESET;
                if (capture.m_offset + capture_substring.size() < subject.size()) {
                    subpatt += subject.substr(capture.m_offset + capture_substring.size());
                }
                std::cout << subpatt << std::endl;
            }

            debugvalue(debuglevel, "OVECTOR", "");
            std::cout << "[";
            size_t si = 0;
            for(auto capture: captures) {
                const std::string capture_substring(subject.substr(capture.m_offset, capture.m_length));
                std::cout << capture.m_offset << ", " << capture.m_offset + capture_substring.size() << ((si++ < captures.size()-1) ? ", " : "");
            }
            std::cout << "]" << std::endl;
        }
        else {
            retval.reverse();
            for(auto s: retval) {
                std::string subpatt = "";
                if (s.offset() > 0) {
                    subpatt += subject.substr(0, s.offset());
                }
                subpatt += BOLDGREEN + s.str() + RESET;
                if (s.offset() + s.str().size() < subject.size()) {
                    subpatt += subject.substr(s.offset() + s.str().size());
                }
                std::cout << subpatt << std::endl;
            }

            debugvalue(debuglevel, "OVECTOR", "");
            std::cout << "[";
            size_t si = 0;
            for(auto s: retval) {
                std::cout << s.offset() << ", " << s.offset() + s.str().size() << ((si++ < retval.size()-1) ? ", " : "");
            }
            std::cout << "]" << std::endl;
        }
    }
    // end debug

    if (time_limit > 0.0) {
        if (((double)ts_diff.tv_sec + ((double)(ts_diff.tv_nsec))/1000000000.0) > time_limit) {
            return EXIT_FAILURE;
        }
    }

    delete(re);

    return EXIT_SUCCESS;
}

