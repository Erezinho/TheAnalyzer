# TheAnalyzer
The Analyzer project goal is to detect clusters of points in n dimensional space and project it to m dimensional space (typically m=2).

I wrote this project as a POC during 2011 and decided to revive and share.

The project is based on MFC library both for both UI purposes and Threading. I'm planing to release a newer version, based on C++ only (and probably Qt for the UI) to get it portable and cross-platform.

The algoritms are based on the 'Fuzzy Clustering and Data Analysis Toolbox'[1]

(A) Gustafsson and Kessel clustering algorithm

(B) Fuzzy Sammon projection algorithm

Implementation is based on the Eigen library - a C++ template library for linear algebra.

Back then I used the Matlab implemetation of those algorithms simulations and testing of my own C++ code.

[1] Balasko, B., Abonyi, J., & Feil, B. (2005). Fuzzy clustering and data analysis toolbox. Department of Process Engineering, University of Veszprem, Veszprem.‏ (http://www.academia.edu/download/34400988/fuzzyclusteringtoolbox.pdf)
