
LIST    =\
	baselining.aux\
	baselining.bbl\
	baselining.blg\
	baselining.log\
	baselining.out\
	baselining.pdf\
	baselining.toc\
	comment.cut

all: pdf bib
	pdflatex baselining

pdf:
	pdflatex baselining

bib: baselining.pdf
	bibtex baselining

clean:
	rm -f ${LIST}

baselining.pdf: baselining.tex baselining.sty
	pdflatex baselining

baselining.aux: baselining.tex baselining.sty
	pdflatex baselining

one:
	pdflatex baselining
	bibtex baselining
	pdflatex baselining
	pdflatex baselining

flip: # Flip to screen-oriented layout.
	perl -p -i -e 's/(RequirePackage.)print,/$$1screen,/g;' baselining.sty

flop: # Flop to default layout.
	perl -p -i -e 's/(RequirePackage.)screen,/$$1print,/g;' baselining.sty

