# -n 0 means to show all, other values means, times from 0% to 100%
gprof ./rpkProfile | gprof2dot -n 1 > callgraph.dot
dot -Tpng callgraph.dot -Ocallgraph.png
dot -Tsvg callgraph.dot -Ocallgraph.svg
