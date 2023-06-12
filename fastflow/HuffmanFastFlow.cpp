#include <vector>
#include <ff/config.hpp>
#if !defined(HAS_CXX11_VARIADIC_TEMPLATES)
#define HAS_CXX11_VARIADIC_TEMPLATES 1
#endif
#include <ff/parallel_for.hpp>
#include <ff/ff.hpp>
#include <ff/map.hpp>


using namespace ff;
const long MYSIZE = 100;

typedef  std::vector<long> ff_task_t;

#ifdef min
#undef min //MD workaround to avoid clashing with min macro in minwindef.h
#endif

struct mapWorker : ff_Map<ff_task_t> {
    ff_task_t *svc(ff_task_t *) {
        ff_task_t *A = new ff_task_t(MYSIZE);
       
        // this is the parallel_for provided by the ff_Map class
        parallel_for(0,A->size(),[&A](const long i) { 
                A->operator[](i)=i;
		}, std::min(3, (int)ff_realNumCores()));
        ff_send_out(A);
        return EOS;
    }
};


struct mapStage: ff_Map<ff_task_t> {
    ff_task_t *svc(ff_task_t *inA) {
        ff_task_t &A = *inA;
        // this is the parallel_for provided by the ff_Map class
        parallel_for(0,A.size(),[&A](const long i) { 
                A[i] += i;
            },2);
        
        printf("mapStage received:\n");
        for(size_t i=0;i<A.size();++i)
            printf("%ld ", A[i]);
        printf("\n");
        delete inA;
        return GO_ON;
    }    
};


int main() {
    
    // farm having map workers
    ff_Farm<ff_task_t, ff_task_t> farm( []() {
            std::vector<std::unique_ptr<ff_node> > W;	
            for(size_t i=0;i<2;++i) W.push_back(make_unique<mapWorker>());
            return W;
        }() );
    
    mapStage stage;
    ff_Pipe<> pipe(farm,stage);
    if (pipe.run_and_wait_end()<0)
        error("running pipe");   
    return 0;	
}