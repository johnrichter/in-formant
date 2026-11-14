#ifndef ANALYSIS_LINPRED_H
#define ANALYSIS_LINPRED_H

#include "rpcxx.h"

namespace Analysis {
    
    class LinpredSolver {
    public:
        virtual ~LinpredSolver() {}
        virtual rpm::vector<double> solve(const double *x, int length, int lpcOrder, double *gain) = 0;
    };

    namespace LP {
        class Autocorr : public LinpredSolver {
        public:
            rpm::vector<double> solve(const double *x, int length, int lpcOrder, double *gain) override;
        private:
            rpm::vector<double> r, a, rc;
        };

        class Covar : public LinpredSolver {
        public:
            rpm::vector<double> solve(const double *x, int length, int lpcOrder, double *gain) override;
        private:
            rpm::vector<double> r, a, rc;
            // The work buffers of one solver. Two solvers never share them.
            rpm::vector<double> b, grc, beta, cc;
        };

        class Burg : public LinpredSolver {
        public:
            rpm::vector<double> solve(const double *x, int length, int lpcOrder, double *gain) override;
        private:
            rpm::vector<double> b1, b2, aa;
        };
    }

}

#endif // ANALYSIS_LINPRED_H
