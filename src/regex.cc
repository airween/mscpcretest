/*
 * original source: libmodsecurity3
 * https://github.com/SpiderLabs/ModSecurity/blob/v3/master/src/utils/regex.h
 */

#include "regex.h"

#include <string>


#if PCRE_CONFIG_JIT
#define pcre_study_opt PCRE_STUDY_JIT_COMPILE
#else
#define pcre_study_opt 0
#endif

void debugvalue(int debuglevel, std::string label, std::string value) {
    if (debuglevel == 1) {
        std::cout << std::endl << label << ":" << std::endl;
        std::cout << std::string(label.size()+1, '=') << std::endl;
        if (value.size() > 0) {
            std::cout << value << std::endl;
        }
    }
}

Regex::Regex(const std::string& pattern_, int debuglevel)
    : pattern(pattern_),
    m_debuglevel(debuglevel),
    m_ovector {0} {
    const char *errptr = NULL;
    int erroffset;

    if (pattern.empty() == true) {
        pattern.assign(".*");
    }

    m_pc = pcre_compile(pattern.c_str(), PCRE_DOTALL|PCRE_MULTILINE,
        &errptr, &erroffset, NULL);

    m_pce = pcre_study(m_pc, pcre_study_opt, &errptr);

#if PCRE_CONFIG_JIT
    debugvalue(m_debuglevel, std::string("JIT"), std::string("avaliable and used"));
    int pijit;
    int rc = pcre_fullinfo(m_pc, m_pce, PCRE_INFO_JIT, &pijit);
    if ((rc != 0) || (pijit != 1)) {
        std::cout << "Regex does not support JIT" << std::endl;
    }
#else
    debugvalue(m_debuglevel, std::string("JIT"), std::string("not avaliable"));
#endif
}


Regex::~Regex() {
    if (m_pc != NULL) {
        pcre_free(m_pc);
        m_pc = NULL;
    }
    if (m_pce != NULL) {
#if PCRE_CONFIG_JIT
        pcre_free_study(m_pce);
#else
        pcre_free(m_pce);
#endif
        m_pce = NULL;
    }
}


std::list<SMatch> Regex::searchAll(const std::string& s) {
    const char *subject = s.c_str();
    const std::string tmpString = std::string(s.c_str(), s.size());
    int ovector[OVECCOUNT];
    int rc, offset = 0;
    int oveccnt = 0;

    std::list<SMatch> retList;

    m_execrc = 0;

    do {
        rc = pcre_exec(m_pc, m_pce, subject,
            s.size(), offset, 0, ovector, OVECCOUNT);

        if (rc > 0) {
            m_execrc += rc;
        }

        for (int i = 0; i < rc; i++) {
            size_t start = ovector[2*i];
            size_t end = ovector[2*i+1];
            size_t len = end - start;
            if (end > s.size()) {
                rc = 0;
                break;
            }

            if (m_debuglevel == 1) {
                // collect substring indexes - not part of original code
                m_ovector[oveccnt++] = ovector[2*i];
                m_ovector[oveccnt++] = ovector[2*i+1];
                // END collect substring indexes
            }
            std::string match = std::string(tmpString, start, len);
            offset = start + len;
            retList.push_front(SMatch(match, start));
            if (len == 0) {
                rc = 0;
                break;
            }
        }
    } while (rc > 0);

    return retList;
}

std::list<SMatch> Regex::searchAll2(const std::string& s) {
    const char *subject = s.c_str();
    const std::string tmpString = std::string(s.c_str(), s.size());
    int ovector[OVECCOUNT];
    int rc, offset = 0;

    std::list<SMatch> retList;

    rc = pcre_exec(m_pc, m_pce, subject,
        s.size(), offset, 0, ovector, OVECCOUNT);

    if (rc > 0) {
        m_execrc = rc;
    }

    for (int i = 0; i < rc; i++) {
        size_t start = ovector[2*i];
        size_t end = ovector[2*i+1];
        size_t len = end - start;
        if (end > s.size()) {
            rc = 0;
            break;
        }
        std::string match = std::string(tmpString, start, len);
        offset = start + len;
        retList.push_front(SMatch(match, start));
        if (len == 0) {
            rc = 0;
            break;
        }
    }

    return retList;
}
