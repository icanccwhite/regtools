/*  junctions_annotator.h -- Declarations for `junctions annotate`

    Copyright (c) 2015, The Griffith Lab

    Author: Avinash Ramu <aramu@genome.wustl.edu>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.  */

#ifndef JUNCTIONS_ANNOTATOR_H_
#define JUNCTIONS_ANNOTATOR_H_

#include <iostream>
#include <iterator>
#include "bedFile.h"
#include "common.h"
#include "gtf_parser.h"
#include "junctions_extractor.h"

using namespace std;

//Format of an annotated junction.
struct AnnotatedJunction : BED {
    //set of transcripts that
    //the junction overlaps
    set<string> transcripts_overlap;
    //set of genes that
    //the junction overlaps
    set<string> genes_overlap;
    //set of exons that the junction
    //overlaps
    set<string> exons_skipped;
    //set of acceptor positions junction overlaps
    set<CHRPOS> acceptors_skipped;
    //set of donor positions junction overlaps
    set<CHRPOS> donors_skipped;
    //splice site annotation (D/DA/NA etc)
    string anchor;
    //five prime reference seq
    string splice_site;
    //Is this a known donor
    bool known_donor;
    //Is this a known acceptor
    bool known_acceptor;
    //Is this a known junction
    bool known_junction;
    //Annotation - Exonic/Intronic etc.
    string annotation;
    //Variant related to the junction
    string variant_info;
    //Print the header line
    static void print_header(ostream& out = std::cout, bool variant_info_exists = false) {
        out << "chrom" << "\t" << "start" <<
                "\t" << "end" << "\t" << "name" <<
                "\t" << "score" << "\t" << "strand" <<
                "\t" << "splice_site" << "\t" << "acceptors_skipped" <<
                "\t" << "exons_skipped" << "\t" << "donors_skipped" <<
                "\t" << "anchor" <<
                "\t" << "known_donor" << "\t" << "known_acceptor" << "\t" << "known_junction" <<
                "\t" << "genes" << "\t" << "transcripts";
        if(variant_info_exists) {
            out << "\t" << "variant_info";
        }
        out << "\n";
    }
    //Print out the junction
    void print(ostream &out = std::cout, bool variant_info_exists = false) {
        out << chrom << "\t" << start <<
                "\t" << end << "\t" << name <<
                "\t" << score << "\t" << strand <<
                "\t" << splice_site << "\t" << acceptors_skipped.size() <<
                "\t" << exons_skipped.size() << "\t" << donors_skipped.size() <<
                "\t" << anchor <<
                "\t" << known_donor << "\t" << known_acceptor << "\t" << known_junction;
        //See if any genes overlap the junction
        if(genes_overlap.size()) {
            out << "\t";
            for(set<string>::iterator it = genes_overlap.begin(); it != genes_overlap.end(); ++it) {
                if(it != genes_overlap.begin())
                    out << ",";
                out << *it;
            }
        } else {
            out << "\t" << "NA";
        }
        //See if any transcripts overlap the junction
        if(transcripts_overlap.size()) {
            out << "\t";
            for(set<string>::iterator it = transcripts_overlap.begin(); it != transcripts_overlap.end(); ++it) {
                if(it != transcripts_overlap.begin())
                    out << ",";
                out << *it;
            }
        } else {
            out << "\t" << "NA";
        }
        if(variant_info_exists) {
            out << "\t" << variant_info;
        }
        out << endl;
    }
    //Clear the contents of the junction
    void reset() {
        anchor = string("N");
        annotation = "";
        splice_site = "";
        known_donor = false;
        known_acceptor = false;
        known_junction = false;
        exons_skipped.clear();
        acceptors_skipped.clear();
        donors_skipped.clear();
        transcripts_overlap.clear();
        genes_overlap.clear();
    }
    //constructor
    AnnotatedJunction() {
        reset();
    }
    //constructor
    AnnotatedJunction(string chr1,
                     uint32_t start1,
                     uint32_t end1) {
        chrom = chr1;
        start = start1;
        end = end1;
        reset();
    }
    //Constructor
    AnnotatedJunction(const Junction &j1) {
        chrom = j1.chrom;
        //Note this is start,end and not thick_start, thick_end
        //So we don't have to adjust ends!
        start = j1.start;
        end = j1.end + 1;
        name = j1.name;
        score = j1.score;
        strand = j1.strand;
        fields = j1.fields;
        reset();
    }
};

inline bool operator<(const AnnotatedJunction& lhs, const AnnotatedJunction& rhs) {
  if(lhs.chrom < rhs.chrom )
      return true;
  if(lhs.chrom == rhs.chrom && lhs.start < rhs.start)
      return true;
  if(lhs.chrom == rhs.chrom && lhs.start == rhs.start && lhs.end < rhs.end)
      return true;
  return false;
}

//The class that does all the annotation
//Uses a GTF parser object to annotate a junction.
class JunctionsAnnotator {
    private:
        //Junctions file to be annotated
        BedFile junctions_;
        //Reference FASTA file
        string ref_;
        //skip single exon genes
        bool skip_single_exon_genes_;
        //output stream to output file
        ofstream ofs_;
        //GTF file object
        GtfParser gtf_;
        //File to write output to
        string output_file_;
        //Check for overlap between a transcript and junctions
        //See if the junction we saw is a known junction
        void check_for_overlap(string transcript_id,
                               AnnotatedJunction & junction);
        //Find overlap for transcripts on the positive strand
        bool overlap_ps(const vector<BED> & exons,
                              AnnotatedJunction & j1);
        //Find overlap for transcripts on the positive strand
        bool overlap_ns(const vector<BED> & exons,
                              AnnotatedJunction & j1);
        //Annotate the anchor
        void annotate_anchor(AnnotatedJunction & junction);
    public:
        //Default constructor
        JunctionsAnnotator()
            : ref_("NA")
            , skip_single_exon_genes_(true)
            , output_file_("NA")
        {}
        JunctionsAnnotator(string ref1, GtfParser gp1)
            : ref_(ref1)
            , skip_single_exon_genes_(true)
            , gtf_(gp1)
            , output_file_("NA")
        {}
        JunctionsAnnotator(string bedFile)
            : ref_("NA")
            , skip_single_exon_genes_(true)
            , output_file_("NA")
            , junctions_(bedFile)
        {}
        //Get the GTF file
        string gtf_file();
        //Get ostream object to write output to
        void set_ofstream_object(ofstream &out);
        //Close ostream object
        void close_ofstream();
        //Parse command-line options for this tool
        int parse_options(int argc, char *argv[]);
        //Print default usage
        int usage(ostream& out = cerr);
        //Get the reference bases at a position
        string get_reference_sequence(string position);
        //Get a single line from the junctions file
        bool get_single_junction(BED & line);
        //Get the anchor bases
        void get_splice_site(AnnotatedJunction & line);
        //Open junctions file
        void open_junctions();
        //Close junctions file
        void close_junctions();
        //Extract gtf info
        bool load_gtf();
        //Set the GTF parser
        void set_gtf_parser(GtfParser gp1) {
            gtf_ = gp1;
        }
        //Annotate with gtf
        void annotate_junction_with_gtf(AnnotatedJunction & j1);
        //Adjust the start and end of the junction
        void adjust_junction_ends(BED & line);
};

#endif
