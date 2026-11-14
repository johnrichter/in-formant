#ifndef ANALYSIS_INVGLOT_H
#define ANALYSIS_INVGLOT_H

#include "rpcxx.h"
#include "../linpred/linpred.h"
#include "../fft/fft.h"
#include <array>
#include <memory>
#include <Eigen/Dense>

namespace Analysis {

    struct InvglotResult {
        double sampleRate;
        rpm::vector<double> glotSig;
    };

    class InvglotSolver {
    public:
        virtual ~InvglotSolver() {}
        virtual InvglotResult solve(const double *x, int length, double sampleRate) = 0;
    };

    namespace Invglot {
        class IAIF : public InvglotSolver {
        public:
            IAIF(double d);
            InvglotResult solve(const double *x, int length, double sampleRate) override;
        private:
            std::unique_ptr<LinpredSolver> lpc;
            double d;
            // The state of one solver. Two solvers, or two threads, never share it.
            rpm::vector<double> lpcIn;
            rpm::vector<double> window;
            rpm::vector<std::array<double, 6>> hpfilt;
            double hpfiltRate = 0.0;
        };

        class GFM_IAIF : public InvglotSolver {
        public:
            GFM_IAIF(double d);
            InvglotResult solve(const double *x, int length, double sampleRate) override;
        private:
            std::unique_ptr<LinpredSolver> lpc;
            double d;
            // The state of one solver. Two solvers, or two threads, never share it.
            rpm::vector<double> lpcIn;
            rpm::vector<double> window;
            rpm::vector<std::array<double, 6>> hpfilt;
            double hpfiltRate = 0.0;
        };

        class AMGIF : public InvglotSolver {
        public:
            AMGIF(int J);
            InvglotResult solve(const double *x, int length, double sampleRate) override;
        private:
            void computeC();
            rpm::vector<Eigen::MatrixXd> C;
            IAIF iaif;
            RealFFT fft;
            int Jmax;
        };
    }

}

#endif // ANALYSIS_INVGLOT_H
