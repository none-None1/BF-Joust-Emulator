/*
Modification of juiced used for importing.
Original author: esolang user ais523
Modified by: esolang user None1
*/
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <cstring>
#include <string>
#include <vector>
#include <iostream>
#include <stdexcept>
using namespace std;
#ifdef __EMSCRIPTEN__
#include <emscripten/bind.h>
#endif
/* A pointer to a nullary command or a loop structure */
typedef void* command;

/* A list of commands, with a backref to the loop that contains it (if
   any) at the end of the list */
typedef command* program;

const char nullary[5] = {};
#define INC ((command)(nullary+0))
#define DEC ((command)(nullary+1))
#define BACK ((command)(nullary+2))
#define FORWARD ((command)(nullary+3))
#define NOP ((command)(nullary+4))

typedef struct loop {
  signed curcount; /* the only bit that changes during the program */
  signed maxcount; /* -1 for [], -2 for {}, or loop count */
  program inner;
  struct loop *otherpart; /* the {} for a ()% or the ()% for a {} */
  command* backref; /* the pointer to this loop in its program */
} loop;

typedef struct looplist {
  loop* data;
  struct looplist* next;
} looplist;

void* mymalloc(size_t length) {
  /* unsigned long long is used here for alignment reasons; anything
     smaller and we can't guarantee that the next element is correctly
     aligned */
  void* m = malloc(length + sizeof (unsigned long long));
  if (!m) {
    //perror("Could not allocate memory");
    throw runtime_error("Something smells fishy!");
  }
  *(unsigned long long*)m = length;
  return (void*)(((unsigned long long*)m)+1);
}

void myfree(void* m) {
  free(((unsigned long long*)m)-1);
}

void* ensurelength(void* m, size_t length) {
  while (((unsigned long long*)m)[-1] < length) {
    ((unsigned long long*)m)[-1] *= 2;
    ((unsigned long long*)m)[-1] += 12;
    m = realloc(((unsigned long long*)m)-1,
                ((unsigned long long*)m)[-1] + sizeof (unsigned long long));
    if (!m) {
      //perror("Could not reallocate memory");
      throw runtime_error("Something smells fishy!");
    }
    m = ((unsigned long long*)m)+1;
  }
  return m;
}

program parseprogram(char*&in, loop* backref, looplist* loopstack) {
  program p = (program)mymalloc(sizeof (command));
  program p2;
  command* q = p;
  int c, n, f;
  for (;;) {
    f = 1;
    c = *in;
    if(!c)c=EOF;
    ++in;
    switch (c) {
    case EOF:
      if (backref != NULL) {
        //fprintf(stderr, "End of file inside loop\n");
        throw runtime_error("Something smells fishy!");
      }
      *q = backref;
      return p;
    case '+': *q = INC; break;
    case '-': *q = DEC; break;
    case '<': *q = BACK; break;
    case '>': *q = FORWARD; break;
    case '.': *q = NOP; break;
    case '[': case '{': case '(': {
        loop* l = (loop*)mymalloc(sizeof (loop));
        looplist ll = {l, loopstack};
        looplist* llp = &ll;
        l->curcount = 0;
        l->maxcount = c == '[' ? -1 : c == '{' ? -2 : 0;
        l->otherpart = NULL;
        if (c == '{') {
          if (!loopstack) {
            //fprintf(stderr, "{} with no matching ()\n");
            throw runtime_error("Something smells fishy!");
          }
          if (loopstack->data->otherpart) {
            //fprintf(stderr, "Multiple {} in one ()\n");
            throw runtime_error("Something smells fishy!");
          }
          loopstack->data->otherpart = l;
          l->otherpart = loopstack->data;
          /* in (({{}})), the outer () matches the inner {} */
          llp = loopstack->next;
        } else if (c == '[') llp = loopstack;
        l->backref = q;
        *q = l;
        l->inner = parseprogram(in, l, llp);
        break;
      }
    case ']':
      if (!backref || backref->maxcount != -1) {
        //fprintf(stderr, "Unmatched ]\n");
        throw runtime_error("Something smells fishy!");
      }
      *q = backref;
      return p;
    case '}':
      if (!backref || backref->maxcount != -2) {
        //fprintf(stderr, "Unmatched }\n");
        throw runtime_error("Something smells fishy!");
      }
      *q = backref;
      return p;
    case ')':
      c = *(in++);
      while (c == ' ' || c == '\n' || c == '\t') c = *(in++);
      if (c != '*' && c != '%') {
        //fprintf(stderr, ") not followed by %% or *\n");
        throw runtime_error("Something smells fishy!");
      }
      if (!backref || backref->maxcount) {
        //fprintf(stderr, "Unmatched )\n");
        throw runtime_error("Something smells fishy!");
      }
#if defined(STRICT_PERCENT_STAR)
      if (backref->otherpart && c == '*') {
        //fprintf(stderr, "{} found inside ()* loop\n");
        throw runtime_error("Something smells fishy!");
      }
      if (!backref->otherpart && c == '%') {
        //fprintf(stderr, "no {} found inside ()%% loop\n");
        throw runtime_error("Something smells fishy!");
      }
#endif
      n = f = 0;
      for (;;) {
        c = *(in++);
        if (c == EOF) {
          throw runtime_error("Something smells fishy!");
        } else if (c >= '0' && c <= '9') {
          n *= 10;
          n += (c - '0');
          f = 1;
        } else if (c == '-' && !f) {
          n = 1000000; /* infinite loop */
        } else if (!isspace(c) || f) {
         *(--in)=c;
          break;
        }
      }
      backref->maxcount = n;
      *q = backref;
      f = 1;
      return p;
    default: f = 0; break;
    }
    q += f;
    p2 = (program)ensurelength(p, (q-p+1) * sizeof (command));
    if (p != p2) {
      command* r = p2;
      q = p2 + (q - p);
      /* We need to update backreferences from loops to the old
         location of p. (The backrefs from inner programs go to the
         loop structs, which weren't just moved in memory.) */
      while (r < q) {
        if (*r != INC && *r != DEC && *r != BACK && *r != FORWARD && *r != NOP) {
          ((loop*)*r)->backref = p2 + (((loop*)*r)->backref - p);
        }
        r++;
      }
      p = p2;
    }
  }
}

program loadprogram(char* c) {
  program p;
  char *d=(char*)malloc(strlen(c)+1);
  char *e=d;
  strcpy(d,c);
  p = parseprogram(d, NULL, NULL);
  free(e);
  return p;
}
struct result{
	string result;
	string reason;
	vector<vector<int> >trace;
};
result run_code(int tapelen,int polarity,char* lf,char *rg,bool trace) {
  program leftprogram, rightprogram;
  command *leftip, *rightip;
  uint8_t tape[30];
  int stepcount;
  int tlfixed = 1;
  uint8_t *leftdata, *rightdata;
  const char *leftlossreason="?", *rightlossreason="?";
  int leftloss=0, rightloss=0;
  int score = 0;
  result final_result;
/*
  if (argc <= 2) {
    //fprintf(stderr, "Usage: %s leftprogram rightprogram [tapelength] [debug]\n",
            *argv ? *argv : "juiced");
    //fprintf(stderr, "Specify anything in the fourth argument to debug.\n");
    //fprintf(stderr,
            "A negative tape length reverses polarity of one program.\n");
    return EXIT_FAILURE;
  }

  if (argc >= 4) {tapelen = atoi(argv[3]); tlfixed = 1;}
  if (tapelen < 0) {tapelen = -tapelen; polarity = -1;}
  if (tapelen < 10 || tapelen > 30) {
    //fprintf(stderr, "Tape length outside range 10..30\n");
    return EXIT_FAILURE;
  }

  while (strchr(argv[1],'/')) argv[1] = strchr(argv[1],'/')+1;
  while (strchr(argv[2],'/')) argv[2] = strchr(argv[2],'/')+1;
  
  while (strlen(argv[1]) + strlen(argv[2]) > 60) {
    if (strlen(argv[1]) > strlen(argv[2]))
      argv[1][strlen(argv[1])-1] = 0;
    else
      argv[2][strlen(argv[2])-1] = 0;
  }*/
  leftprogram = loadprogram(lf);
  rightprogram = loadprogram(rg);

nextrun:
   //printf("%*s%s vs %s: ",
                        //60 - strlen(argv[1]) - strlen(argv[2]), "",
                        //argv[1], argv[2]);
  for (leftdata = tape; leftdata < tape+tapelen; leftdata++)
    *leftdata = 0;
  tape[0] = 128; tape[tapelen-1] = 128;
  leftdata = tape; rightdata = tape + (tapelen-1);
  leftip = leftprogram;
  rightip = rightprogram;
  leftloss = 0;
  rightloss = 0;
  leftlossreason = "?";
  rightlossreason = "?";
  vector<int>new_trace;
	new_trace.push_back(0);
	new_trace.push_back(leftdata-tape);
	new_trace.push_back(rightdata-tape);
	new_trace.push_back(0); // juiced doesn't support displaying current IP, so I'll leave these two blank
	new_trace.push_back(0);
	for(int i = 0; i < tapelen; i++)new_trace.push_back(tape[i]);
	final_result.trace.push_back(new_trace);
  for (stepcount = 0; stepcount < 100000; stepcount++) {
    uint8_t leftdatavalue = *leftdata;
    uint8_t rightdatavalue = *rightdata;
    for (;;) { /* until left takes a turn */
      if (*leftip == INC) {*leftdata += polarity; break;}
      else if (*leftip == DEC) {*leftdata -= polarity; break;}
      else if (*leftip == NOP) break;
      else if (!*leftip) {leftip--; break;}
      else if (*leftip == BACK) {
        if (leftdata == tape) {
          leftlossreason = "off own end";
          leftloss = 3;
        } else leftdata--;
        break;
      } else if (*leftip == FORWARD) {
        if (leftdata == tape + (tapelen-1)) {
          leftlossreason = "off opponent's end";
          leftloss = 3;
        } else leftdata++;
        break;
      } else {
        loop* l = (loop*) *leftip;
        if (leftip != l->backref && l->maxcount == -2) {
          /* We reached the end of a {} loop; reset the count of the
             containing () and move to the command after. This
             behaviour's needed for nested {} to work correctly. */
          leftip = l->backref + 1;
          l->otherpart->curcount = l->otherpart->maxcount;
        } else if (l->maxcount == -2) {
          /* We reached the start of a {} loop; enter it if the
             containing ()% is at maximum count, and otherwise jump
             back to the start of the containing loop */
          if (l->otherpart->curcount == l->otherpart->maxcount)
            leftip = l->inner;
          else
            leftip = l->otherpart->inner;
          l->otherpart->curcount++;
        } else {
          /* If entering the loop from the start, reset the count
          There are two purposes to this: to get nested {} right,
          and so that we can run programs repeatedly */
          if (leftip == l->backref) {
            l->curcount = 0;
            if (*(l->inner) == *leftip && l->maxcount >= 0) {
              /* ()*n loops produce debug output */
              fflush(stdout);
              //fprintf(argc == 5 ? stdout : stderr, "()*%d@%d%c",
                      //l->maxcount, tapelen, argc == 5 ? ' ' : '\n');
              fflush(stderr);
            }
          }
          if ((l->maxcount == -1 && leftdatavalue) ||
              (l->maxcount >= 0 && !l->otherpart &&
               l->curcount != l->maxcount) ||
              (l->otherpart && l->curcount > 1) ||
              (l->otherpart && leftip == l->backref && l->maxcount)) {
            /* Otherwise it doesn't matter whether we're entering from the
               start of the loop or not; just see if we should enter it
               Exception: the start of a ({}) loop increments not
               decrements its count, and enters the start not the middle */
            if (l->otherpart && leftip != l->backref) {
              leftip = l->otherpart->backref+1;
              l->curcount--;
            } else {
              leftip = l->inner;
              l->curcount++;
            }
          } else {
            leftip = l->backref + 1;
            l->curcount = 0;
          }
          /* [ and ] take a turn each */
          if (l->maxcount == -1) {leftip--; break;}
        }
      }
    }
    leftip++;
    for (;;) { /* until right takes a turn */
      if (*rightip == INC) {*rightdata += 1; break;}
      else if (*rightip == DEC) {*rightdata -= 1; break;}
      else if (*rightip == NOP) break;
      else if (!*rightip) {rightip--; break;}
      else if (*rightip == BACK) {
        if (rightdata == tape + (tapelen-1)) {
          rightlossreason = "off own end";
          rightloss = 3;
        } else rightdata++;
        break;
      } else if (*rightip == FORWARD) {
        if (rightdata == tape) {
          rightlossreason = "off opponent's end";
          rightloss = 3;
        } else rightdata--;
        break;
      } else {
        loop* l = (loop*) *rightip;
        if (rightip != l->backref && l->maxcount == -2) {
          /* We reached the end of a {} loop; reset the count of the
             containing () and move to the command after. This
             behaviour's needed for nested {} to work correctly. */
          rightip = l->backref + 1;
          l->otherpart->curcount = l->otherpart->maxcount;
        } else if (l->maxcount == -2) {
          /* We reached the start of a {} loop; enter it if the
             containing ()% is at maximum count, and otherwise jump
             back to the start of the containing loop */
          if (l->otherpart->curcount == l->otherpart->maxcount)
            rightip = l->inner;
          else
            rightip = l->otherpart->inner;
          l->otherpart->curcount++;
        } else {
          /* If entering the loop from the start, reset the count
             There are two purposes to this: to get nested {} right,
             and so that we can run programs repeatedly */
          if (rightip == l->backref) {
            l->curcount = 0;
            if (*(l->inner) == *rightip && l->maxcount >= 0) {
              /* ()*n loops produce debug output */
              fflush(stdout);
              //fprintf(argc == 5 ? stdout : stderr, "()*%d@%d%c",
                      //l->maxcount, tapelen, argc == 5 ? ' ' : '\n');
              fflush(stderr);
            }
          }
          if ((l->maxcount == -1 && rightdatavalue) ||
              (l->maxcount >= 0 && !l->otherpart &&
               l->curcount != l->maxcount) ||
              (l->otherpart && l->curcount > 1) ||
              (l->otherpart && rightip == l->backref && l->maxcount)) {
            /* Otherwise it doesn't matter whether we're entering from the
               start of the loop or not; just see if we should enter it
               Exception: the start of a ({}) loop increments not
               decrements its count, and enters the start not the middle */
            if (l->otherpart && rightip != l->backref) {
              rightip = l->otherpart->backref+1;
              l->curcount--;
            } else {
              rightip = l->inner;
              l->curcount++;
            }
          } else {
            rightip = l->backref + 1;
            l->curcount = 0;
          }
        }
        /* [ and ] take a turn each */
        if (l->maxcount == -1) {rightip--; break;}
      }
    }
    rightip++;

    /* This bit was inspired by egojoust, and partially copied from
       there */
    if (tape[0] == 0) {
      if (leftloss) leftlossreason = "flag fell";
      leftloss++;
    } else if (leftloss < 3) {
      leftloss = 0;
    }
    if (tape[tapelen-1] == 0) {
      if (rightloss) rightlossreason = "flag fell";
      rightloss++;
    } else if (rightloss < 3) {
      rightloss = 0;
    }

    if (trace) {
    	vector<int>new_trace;
    	new_trace.push_back(stepcount+1);
    	new_trace.push_back(leftdata-tape);
    	new_trace.push_back(rightdata-tape);
    	new_trace.push_back(0); // juiced doesn't support displaying current IP, so I'll leave these two blank
    	new_trace.push_back(0);
    	for(int i = 0; i < tapelen; i++)new_trace.push_back(tape[i]);
    	final_result.trace.push_back(new_trace);
      //printf("\n%7d: ", stepcount);
      /*for(i = 0; i < tapelen; i++) {
        if (i == 0); //printf("\x1b[34m");
        else if (i == tapelen-1); //printf("\x1b[31m");
        if (i == leftdata-tape && i == rightdata-tape); //printf("\x1b[35;1m");
        else if (i == leftdata-tape); //printf("\x1b[34;1m");
        else if (i == rightdata-tape); //printf("\x1b[31;1m");
        if (tape[i]); //printf("%02X ",(unsigned char)tape[i]);
        //printf(".. ");
        //printf("\x1b[0m");
      }*/
    }

    if (leftloss >= 2 || rightloss >= 2) break;
  }


  //printf("%d (%s): ", tapelen, polarity == 1 ? "standard" : "switched");
  if (leftloss == 1) leftloss = 0;
  if (rightloss == 1) rightloss = 0;
  if (leftloss > rightloss) {
    //printf("Left loses (%s)!\n", leftlossreason);
    final_result.result='X';
    final_result.reason=leftlossreason;
    score++;
  } else if (rightloss > leftloss) {
    //printf("Right loses (%s)!\n", rightlossreason);
    final_result.result='Y';
    final_result.reason=rightlossreason;
    score--;
  } else if (!rightloss) {
    //printf("Tie (timeout)!\n");
    final_result.result='T';
    final_result.reason="timeout";
  } else {
  	final_result.result='T';
    final_result.reason="left loss: "+string(leftlossreason)+", right loss: "+string(rightlossreason);
    //printf("Tie (left loss: %s; right loss: %s)!\n",
           //leftlossreason, rightlossreason);
  }

  if (!tlfixed) {
    if (tapelen < 30) tapelen++;
    else if (polarity == 1) {tapelen = 10; polarity = -1;}
    if (tapelen == 30 && polarity == -1) tlfixed = 1;
    goto nextrun;
  }
  return final_result;
}
result bfjoust(string left,string right,int tapelen,int switched,int trace){
#ifdef __EMSCRIPTEN__
	try{
#endif
		return run_code(tapelen,switched?-1:1,(char*)left.c_str(),(char*)right.c_str(),trace);
#ifdef __EMSCRIPTEN__
	}catch(...){
		result final_result;
		final_result.result='E';
		final_result.reason="Something smells fishy!";
		return final_result;
	}
#endif
}
#ifdef __EMSCRIPTEN__
EMSCRIPTEN_BINDINGS(my_module) {
	emscripten::register_vector<int>("vector_int");
	emscripten::register_vector<std::vector<int>>("vector_vector_int");
    emscripten::value_object<result>("Result")
        .field("result", &result::result)
        .field("reason", &result::reason)
        .field("trace", &result::trace);

    emscripten::function("bfjoust", &bfjoust);
}
#else
int main(){
	result res=bfjoust(">>","+<",20,0,1);
	cout<<res.result<<' '<<res.reason<<'\n';
	for(auto i:res.trace){
		for(auto j:i){
			cout<<j<<' ';
		}
		cout<<'\n';
	}
}
#endif
