#ifndef FW_BW_H
#define FW_BW_H

#include "wtime.h"
#include "util.h"
#include "graph.h"
#include <set>
#include <iostream>

inline void fw_bfs(
        index_t *scc_id,
        long_t *fw_beg_pos,
        long_t *bw_beg_pos,
        index_t vert_beg,
        index_t vert_end,
        vertex_t *fw_csr,
        vertex_t *bw_csr,
        depth_t *fw_sa,
        index_t *front_comm,
        long_t *work_comm,
        vertex_t root,
        index_t tid,
        index_t thread_count,
        double alpha,
        double beta,
        long_t edge_count,
        vertex_t vert_count,
        vertex_t world_size,
        vertex_t world_rank,
        vertex_t step,
        vertex_t *fq_comm,
        unsigned int *sa_compress,
        vertex_t virtual_count)
{
    depth_t level = 0;
//    int level = 0;
    fw_sa[root] = 0;
    bool is_top_down = true;	
    bool is_top_down_queue = false;
    index_t queue_size = vert_count / 100; //thread_count;
    
    double sync_time_fw = 0;
    while(true)
    {
//        std::cout<<level<<"\n";

        double ltm= wtime();
        double sync_time = 0.0;
        index_t front_count=0;
        signed char is_done = 0;
        long_t my_work_next=0;
        index_t my_work_curr=0;

        if(is_top_down)
        {
            std::cout<<vert_beg<<","<<vert_end<<"\n";
            for(vertex_t vert_id=vert_beg; vert_id<vert_end; vert_id++)
            {
                if(scc_id[vert_id] == 0 && fw_sa[vert_id]==(level))
                {
                    long_t my_beg = fw_beg_pos[vert_id];
                    long_t my_end = fw_beg_pos[vert_id+1];

                    for(; my_beg<my_end; my_beg++)
                    {
                        vertex_t nebr=fw_csr[my_beg];
                        if(scc_id[nebr] == 0 && fw_sa[nebr] == -1)
                        {
                            fw_sa[nebr] = level+1;
                            sa_compress[nebr / 32] |= ((index_t)1 << ((index_t)nebr % 32)); 
//                            std::cout<<((index_t)1 << (31 - nebr % 32))<<"\n";
                            my_work_next += fw_beg_pos[nebr+1]-fw_beg_pos[nebr];
                            fq_comm[front_count] = nebr;
                            front_count++;
                        }
                    }
                }
            }
            std::cout<<my_work_next<<"\n";
//            work_comm[tid]=my_work_next;
        }
        else
            if(!is_top_down_queue)
            {
//                #pragma omp parallel for
                for(vertex_t vert_id=vert_beg; vert_id<vert_end; vert_id++)
                {
                    if(scc_id[vert_id] == 0 && fw_sa[vert_id]== -1)
                    {
                        long_t my_beg = bw_beg_pos[vert_id];
                        long_t my_end = bw_beg_pos[vert_id+1];
                        my_work_next += my_end - my_beg;

                        for(; my_beg<my_end; my_beg++)
                        {
                            vertex_t nebr=bw_csr[my_beg];
                            if(scc_id[vert_id] == 0 && fw_sa[nebr] != -1)
                            {
                                fw_sa[vert_id] = level+1;
                                fq_comm[front_count] = vert_id;
                                sa_compress[vert_id / 32] |= ((index_t)1 << ((index_t)vert_id % 32)); 
                                front_count++;
                                break;
                            }
                        }
                    }
                }

//                int num_sa = 0;
//                for(int i = vert_beg; i < vert_end; ++i)
//                {
//                    //sa_compress[vert_id >> 5] |= (1 << (vert_id & 31)); 
//                    //if(scc_id[i] == 0 && fw_sa[i] == -1 && (sa_compress[i / 32] |= (1 << (i % 32 == 0 ? 0 : i % 32 - 1)))) 
//                    if(scc_id[i] == 0 && fw_sa[i] == -1 && ((sa_compress[i / 32] & ((index_t)1 << (i % 32))) != 0))
//                    //if(scc_id[v_new] == 0 && fw_sa[v_new] == -1)
//                    {
//                        fw_sa[i] = level + 1;
//                        num_sa += 1;
//                    }
//                }
//                std::cout<<"Debug,"<<num_sa<<","<<front_count<<"\n";
//                std::cout<<"_level,"<<(int)level<<"tid,"<<tid<<"\n";
//                work_comm[tid]=my_work_next;
                
            }
            else
            {
//                printf("queue_size = %d\n", queue_size);
                index_t *q = new index_t[queue_size];
                index_t head = 0;
                index_t tail = 0;
                //std::queue<index_t> q;

//                //Option 1: put current level vertices into fq
//                if(level % 2 == 0)
//                {
//                    for(vertex_t vert_id=vert_beg; vert_id<vert_end; vert_id++)
//                    {
//                        if(scc_id[vert_id] == 0 && (fw_sa[vert_id] == level || fw_sa[vert_id] == level - 1))
//                        {
//                            q[tail++] = vert_id;
//    //                        if(tail == queue_size)
//    //                            tail = 0;
//                        }
//
//                    }
//                }
//                else
//                {
                    for(vertex_t vert_id=vert_beg; vert_id<vert_end; vert_id++)
                    {
                        if(scc_id[vert_id] == 0 && fw_sa[vert_id] == level)
                        {
                            q[tail++] = vert_id;
    //                        if(tail == queue_size)
    //                            tail = 0;
                        }

                    }
//                }
//                printf("tail = %d\n", tail);
                while(head != tail)
                {
                    vertex_t temp_v = q[head++];
                    if(head == queue_size)
                        head = 0;
                    long_t my_beg = fw_beg_pos[temp_v];
                    long_t my_end = fw_beg_pos[temp_v+1];

                    for(; my_beg < my_end; ++my_beg)
                    {
                        index_t w = fw_csr[my_beg];
                        
                        if(scc_id[w] == 0 && fw_sa[w] == -1)
                        {
                            q[tail++] = w;
                            if(tail == queue_size)
                                tail = 0;
                            fw_sa[w] = level + 1;
                        }
                    }
                }
//                delete[] q;
//                MPI_Barrier(MPI_COMM_WORLD);
//                MPI_Allgather(&fw_sa[vert_beg],
//                    vert_end - vert_beg,
//                    MPI_INT,
//                    fw_sa,
//                    vert_count,
//                    MPI_INT,
//                    MPI_COMM_WORLD);
                double temp_time = wtime();
                MPI_Allreduce(MPI_IN_PLACE,
                        fw_sa,
                        vert_count,
                        MPI_SIGNED_CHAR,
                        MPI_MAX,
                        MPI_COMM_WORLD);
                sync_time += wtime() - temp_time;
//                std::cout<<"Async done sync,"<<(wtime() - temp_time) * 1000<<" ms\n";
//
//			if(tid==0) std::cout<<"Level-"<<(int)level
//				<<"-frontier-time-futurework(currwork in btup): "
//				<<front_count<<" "
//				<<(wtime() - ltm) * 1000<<" ms "
//				<<my_work_next<<"\n";
//
//                MPI_Barrier(MPI_COMM_WORLD);
                break;
            }
        
//        printf("level = %d\n", level);
        front_comm[tid]=front_count;
        
//        MPI_Barrier(MPI_COMM_WORLD);
        
//        front_comm[tid] = front_count;
//        work_comm[tid] = my_work_next;

        double temp_time = wtime();
        

//        if(front_count != 0)
//            is_done = 1;
//
//        MPI_Allreduce(MPI_IN_PLACE,
//                &is_done,
//                1,
//                MPI_SIGNED_CHAR,
//                MPI_LAND,
//                MPI_COMM_WORLD);
//
        if(is_top_down)
        {
//            printf("before, %ld\n", my_work_next);
            MPI_Allreduce(MPI_IN_PLACE,
                    &my_work_next,
                    1,
                    MPI_LONG,
                    MPI_SUM,
                    MPI_COMM_WORLD);
//            printf("after\n");
        }

        MPI_Allreduce(MPI_IN_PLACE,
                &front_count,
                1,
                MPI_INT,
                MPI_SUM,
                MPI_COMM_WORLD);
        

//        MPI_Allgather(&fw_sa[vert_beg],
//            step,
//            MPI_INT,
//            front_comm,
//            step,
//            MPI_INT,
//            MPI_COMM_WORLD);
//
//        std::cout<<front_comm[tid]<<"\n";
//
//        double temp_time = wtime();
//        MPI_Allgather(&front_comm[tid],
//            1,
//            MPI_INT,
//            front_comm,
//            1,
//            MPI_INT,
//            MPI_COMM_WORLD);
//
////        std::cout<<tid<<",front_count,"<<front_count<<"\n";
//
////        MPI_Allreduce(MPI_IN_PLACE,
////                &my_work_next,
////                1,
////                MPI_INT,
////                MPI_SUM,
////                MPI_COMM_WORLD);
//
//        MPI_Allgather(&work_comm[tid],
//            1,
//            MPI_INT,
//            work_comm,
//            1,
//            MPI_INT,
//            MPI_COMM_WORLD);
//

//        std::cout<<"sync two frontier size,"<<level<<",tid"<<tid<<",time,"<<(wtime() - temp_time) * 1000 <<" ms\n";
        sync_time += wtime() - temp_time;

//        front_count = 0;
//        for(int i = 0; i < thread_count; ++i)
//        {
//            front_count += front_comm[i];
//        }
//
//        my_work_next = 0;
//        for(int i = 0; i < thread_count; ++i)
//        {
//            my_work_next += work_comm[i];
//        }

        if(front_count == 0) 
//        if(is_done > 0) 
        {
            double temp_time = wtime();
            MPI_Allreduce(MPI_IN_PLACE,
                    fw_sa,
                    vert_count,
                    MPI_SIGNED_CHAR,
                    MPI_MAX,
                    MPI_COMM_WORLD);
            std::cout<<"FW final sync,"<<(int)level<<",tid"<<tid<<",time,"<<(wtime() - temp_time) * 1000 <<" ms\n";
            break;
        }

//        if(level > 2 || (is_top_down && (my_work_next * alpha > edge_count)))

        if((is_top_down && (my_work_next * alpha > edge_count)))
        {
            is_top_down=false;
//            double temp_time = wtime();
//
//            MPI_Allreduce(MPI_IN_PLACE,
//                    fw_sa,
//                    vert_count,
//                    MPI_SIGNED_CHAR,
//                    MPI_MAX,
//                    MPI_COMM_WORLD);
////            std::cout<<"td_to_bt Sync fw_sa,"<<(wtime() - sync_time) * 1000<<" ms\n";
            if(tid==0)
            //std::cout<<"--->Switch to bottom-up, level = "<<int(level)<<","<<(wtime() - temp_time) * 1000<<" (ms)<----\n";
                std::cout<<"--->Switch to bottom-up, level = "<<(int)(level)<<"\n";

//            sync_time += wtime() - temp_time;
        }	
        else if(level > 50)
//        else if(!is_top_down && (my_work_next * beta < edge_count) && level % 2 == 0)
//        else if(!is_top_down && (my_work_next * beta < edge_count))
        {
            is_top_down_queue = true;
            double temp_time = wtime();

            MPI_Allgather(&fw_sa[vert_beg],
                step,
                MPI_SIGNED_CHAR,
                fw_sa,
                step,
                MPI_SIGNED_CHAR,
                MPI_COMM_WORLD);
            
//            index_t simple_end = world_size * step;
//            if(vert_count != simple_end)
//            {
////                    printf("%d, %d\n", simple_end, vert_count);
//                MPI_Allreduce(MPI_IN_PLACE,
//                    &fw_sa[simple_end],
//                    vert_count - simple_end,
//                    MPI_SIGNED_CHAR,
//                    MPI_MAX,
//                    MPI_COMM_WORLD);
//            }
            if(tid==0)
                std::cout<<"--->Switch to async top-down, level = "<<int(level)<<"<----\n";
//            std::cout<<"bt Sync fw_sa,"<<(wtime() - sync_time) * 1000<<" ms\n";
//        std::cout<<"level,"<<level<<",tid"<<tid<<",time,"<<(wtime() - temp_time) * 1000 <<" ms\n";
            sync_time += wtime() - temp_time;
        }

//            if((is_top_down_queue || (!is_top_down)) && level % 2 == 1)
            if((is_top_down_queue || (!is_top_down)) )
            {
                double temp_time = wtime();
//                MPI_Allreduce(MPI_IN_PLACE,
//                        fw_sa,
//                        vert_count,
//                        MPI_INT,
//                        MPI_MAX,
//                        MPI_COMM_WORLD);
                if(front_count > 10000)
//                if(front_count > vert_count)
                {
//                    if(tid == 0)
//                    {
//                        std::cout<<"sync fw_sa\n";
//                        std::cout<<front_count<<","<<world_rank<<","<<world_size<<"\n";
//                    }
               //     std::cout<<front_count<<","<<world_rank<<","<<world_size<<"\n";
                    double temp_time = wtime();
//                    MPI_Allgather(&sa_compress[vert_beg / 32],
//                        step / 32,
//                        MPI_UNSIGNED,
//                        sa_compress,
//                        step / 32,
//                        MPI_UNSIGNED,
//                        MPI_COMM_WORLD);
//
                    MPI_Allreduce(MPI_IN_PLACE,
                        sa_compress,
                        virtual_count / 32,
                        //sa_compress,
                        //vert_count / 32 + 32,
                        MPI_UNSIGNED,
                        MPI_BOR,
                        MPI_COMM_WORLD);
                    sync_time += wtime() - temp_time;
//                    
                    vertex_t num_sa = 0;
                    for(index_t i = 0; i < vert_count; ++i)
                    {
                        //sa_compress[vert_id >> 5] |= (1 << (vert_id & 31));  
                        if(scc_id[i] == 0 && fw_sa[i] == -1 && ((sa_compress[i / 32] & ((index_t)1 << ((index_t)i % 32))) != 0))
                        //if(scc_id[v_new] == 0 && fw_sa[v_new] == -1)
                        {
                            fw_sa[i] = level + 1;
                            num_sa += 1;
                        }
                    }
                //    std::cout<<"num_sa,"<<num_sa<<",front_comm,"<<front_comm[world_rank]<<","<<num_sa + front_comm[world_rank]<<"\n";
//                    
//                    MPI_Allgather(&fw_sa[vert_beg],
//                        step,
//                        MPI_SIGNED_CHAR,
//                        fw_sa,
//                        step,
//                        MPI_SIGNED_CHAR,
//                        MPI_COMM_WORLD);
                  
//                    index_t simple_end = world_size * step;
//                    if(vert_count != simple_end)
//                    {
//    //                    printf("%d, %d\n", simple_end, vert_count);
//                        MPI_Allreduce(MPI_IN_PLACE,
//                            &fw_sa[simple_end],
//                            vert_count - simple_end,
//                            MPI_SIGNED_CHAR,
//                            MPI_MAX,
//                            MPI_COMM_WORLD);
//                    }
                }
                else
                {   
                    // Step 0: sync front_comm to get the send buffer size of each processor
//                    if(tid == 0)
//                    {
//                        std::cout<<"sync fq\n";
//                        std::cout<<front_count<<","<<world_rank<<","<<world_size<<"\n";
//                    }
//
//                    printf("before gather\n");
                    MPI_Allreduce(MPI_IN_PLACE,
                            front_comm,
                            world_size,
                            MPI_INT,
                            MPI_MAX,
                            MPI_COMM_WORLD);

//                    MPI_Allgather(&front_comm[world_rank],
//                        1,
//                        MPI_INT,
//                        front_comm,
//                        1,
//                        MPI_INT,
//                        MPI_COMM_WORLD);

//                    printf("after gather\n");
                    // Step 1: send fq_comm to other processors
//                    if(tid == 0)
//                        std::cout<<"sending"<<"\n"; 

                    MPI_Request request;

                    for(int i = 0; i < world_size; ++i)
                    {
//                        std::cout<<i<<","<<front_comm[world_rank]<<"\n"; 
                        if(i != world_rank)
                        {
                            MPI_Isend(fq_comm,
                                    front_comm[world_rank],
                                    MPI_INT,
                                    i,
                                    0,
                                    MPI_COMM_WORLD,
                                    &request);
                            MPI_Request_free(&request);
                        }
                    }

                    // Step 2: recv and store into fq_comm
//                    if(tid == 0)
//                        std::cout<<"receiving"<<"\n"; 
                    
                    vertex_t fq_begin = front_comm[world_rank];
                    for(int i = 0; i < world_size; ++i)
                    {
                        if(i != world_rank)
                        {
                            MPI_Recv(&fq_comm[fq_begin],
                                    front_comm[i],
                                    MPI_INT,
                                    i,
                                    0,
                                    MPI_COMM_WORLD,
                                    MPI_STATUS_IGNORE);
                            fq_begin += front_comm[i];
                        }
                    }
                    sync_time += wtime() - temp_time;

                    // Step 3: recover status array fw_sa from the new fq_comm
//                    if(tid == 0)
//                        std::cout<<"recover"<<"\n"; 
                    //for(int i = front_comm[world_rank]; i < front_count; ++i)
                    for(int i = front_comm[world_rank]; i < front_count; ++i)
                    {
                        vertex_t v_new = fq_comm[i];
                        if(fw_sa[v_new] == -1)
                        {
                            fw_sa[v_new] = level + 1;
                        }
                    }

                }

//                std::cout<<"sync level,"<<(int)level<<",tid"<<tid<<",time,"<<(wtime() - temp_time) * 1000 <<" ms\n";
//                sync_time += wtime() - temp_time;
//                std::cout<<"bt Sync fw_sa,"<<(wtime() - sync_time) * 1000<<" ms\n";

                
    //            MPI_Barrier(MPI_COMM_WORLD);

    //            MPI_Allgatherv(&fw_sa[vert_beg],
    //                vert_end - vert_beg,
    //                MPI_INT,
    //                fw_sa,
    //                recv_counts,
    //                displs,
    //                MPI_INT,
    //                MPI_COMM_WORLD);

    //            if(tid==0) 
            }
//			if(tid==0) std::cout<<"Level-"<<(int)level
//				<<"-frontier-time-futurework(currwork in btup): "
//				<<front_count<<" "
//				<<(wtime() - ltm) * 1000<<" ms "
//				<<my_work_next<<"\n";
			//if(tid==0) std::cout<<"Level-"<<(int)level
			//	<<","<<tid<<","
			//	<<front_count<<" "
            //    <<sync_time * 1000<<" ms,"
			//	<<(wtime() - ltm) * 1000<<" ms "
			//	<<"\n";
            
            if(tid == 0)
                std::cout<<"FW_Level-"<<(int)level
                    <<","<<tid<<","
                    <<front_count<<" "
                    <<sync_time * 1000<<" ms,"
                    <<(wtime() - ltm) * 1000<<" ms "
                    <<"\n";
//
//        
//        MPI_Allreduce(MPI_IN_PLACE,
//                fw_sa,
//                vert_count,
//                MPI_INT,
//                MPI_MAX,
//                MPI_COMM_WORLD);
//
//        MPI_Barrier(MPI_COMM_WORLD);
        level ++;
        sync_time_fw += sync_time;
    }
    std::cout<<"FW comm time,"<<sync_time_fw * 1000<<",(ms)\n";
}

inline void bw_bfs(
        index_t *scc_id,
        long_t *fw_beg_pos,
        long_t *bw_beg_pos,
        index_t vert_beg,
        index_t vert_end,
        vertex_t *fw_csr,
        vertex_t *bw_csr,
        depth_t *fw_sa,
        depth_t *bw_sa,
        index_t *front_comm,
        long_t *work_comm,
        vertex_t root,
        index_t tid,
        index_t thread_count,
        double alpha,
        double beta,
        vertex_t edge_count,
        vertex_t vert_count,
        vertex_t world_size,
        vertex_t world_rank,
        vertex_t step,
        vertex_t *fq_comm,
        unsigned int *sa_compress
        )
{
    bw_sa[root] = 0;
    bool is_top_down = true;
    bool is_top_down_queue = false;
    depth_t level = 0;
    scc_id[root] = 1;
    index_t queue_size = vert_count / 100; //thread_count;

    long_t my_beg = bw_beg_pos[root];
    long_t my_end = bw_beg_pos[root+1];

    double sync_time_bw = 0;

    
// //   printf("vert_beg = %d, vert_end = %d\n", vert_beg, vert_end);
//    for(; my_beg<my_end; my_beg++)
//    {
//        vertex_t nebr=bw_csr[my_beg];
//        if(scc_id[nebr] == 0 && bw_sa[nebr] == -1 && fw_sa[nebr] != -1)
//        {
//            bw_sa[nebr] = level+1;
//     //       my_work_next+=bw_beg_pos[nebr+1]-bw_beg_pos[nebr];
//    //        front_count++;
//            scc_id[nebr] = 1;	
//        }
//    }
//    level += 1;

    while(true)
    {
        double ltm= wtime();
        index_t front_count=0;
        long_t my_work_next=0;
        index_t my_work_curr=0;

        double sync_time = 0.0;
        if(is_top_down)
        {
            for(vertex_t vert_id=vert_beg; vert_id<vert_end; vert_id++)
            {
//                if(scc_id[vert_id] == 1 && bw_sa[vert_id]==level)
                if(bw_sa[vert_id]==level)
                {
                    long_t my_beg = bw_beg_pos[vert_id];
                    long_t my_end = bw_beg_pos[vert_id+1];

//                        printf("my_beg = %d, my_end = %d\n", my_beg, my_end);
                    for(; my_beg<my_end; my_beg++)
                    {
                        vertex_t nebr=bw_csr[my_beg];
//                        if(scc_id[nebr] == 0 && bw_sa[nebr] == -1 && fw_sa[nebr] != -1)
                        if(bw_sa[nebr] == -1 && fw_sa[nebr] != -1)
                        {
                            bw_sa[nebr] = level+1;
                            my_work_next+=bw_beg_pos[nebr+1]-bw_beg_pos[nebr];
                            fq_comm[front_count] = nebr;
                            front_count++;
                            scc_id[nebr] = 1;	
                            sa_compress[nebr / 32] |= ((index_t)1 << ((index_t)nebr % 32)); 

                            
                        }
                    }
                }
            }
            work_comm[tid]=my_work_next;
        }
        else
            if(!is_top_down_queue)
            {
                for(vertex_t vert_id=vert_beg; vert_id<vert_end; vert_id++)
                {
//                    if(scc_id[vert_id] == 0 && bw_sa[vert_id] == -1 && fw_sa[vert_id] != -1)
                    if(bw_sa[vert_id] == -1 && fw_sa[vert_id] != -1)
                    {
                        long_t my_beg = fw_beg_pos[vert_id];
                        long_t my_end = fw_beg_pos[vert_id+1];
                        my_work_next+=my_end-my_beg;

                        for(; my_beg<my_end; my_beg++)
                        {
                            vertex_t nebr=fw_csr[my_beg];
//                            if(scc_id[nebr] == 1 && bw_sa[nebr] != -1)//fw_sa[nebr] != -1)
                            if(bw_sa[nebr] != -1)//fw_sa[nebr] != -1)
                            {
                                bw_sa[vert_id] = level+1;
                                fq_comm[front_count] = vert_id;
                                front_count++;
                                sa_compress[vert_id / 32] |= ((index_t)1 << ((index_t)vert_id % 32)); 
                                scc_id[vert_id] = 1;
                                break;
                            }
                        }
                    }
                }
                work_comm[tid]=my_work_next;
            }
            else
            {
//                std::queue<index_t> q;
                index_t *q = new index_t[queue_size];
                index_t head = 0;
                index_t tail = 0;
                
				for(vertex_t vert_id=vert_beg; vert_id<vert_end; vert_id++)
                {
//                    if(scc_id[vert_id] == 1 && bw_sa[vert_id] == level)
                    if(bw_sa[vert_id] == level)
                    {
//                        q.push(vert_id);
                        q[tail++] = vert_id;
                        //impossible here
//                        if(tail == queue_size)
//                            tail = 0;
                    }
                }

//                printf("queue_size = %d, tail = %d\n", queue_size, tail);
//                printf("q[%d].size = %d\n", tid, q.size());
                while(head != tail)
                {
//                    index_t temp_v = q.front();
//                    q.pop();
                    vertex_t temp_v = q[head++];
                    if(head == queue_size)
                        head = 0;
                    long_t my_beg = bw_beg_pos[temp_v];
                    long_t my_end = bw_beg_pos[temp_v+1];

                    for(; my_beg < my_end; ++my_beg)
                    {
                        index_t w = bw_csr[my_beg];
//                        if(scc_id[w] == 0 && bw_sa[w] == -1 && fw_sa[w] != -1)
                        
                        if(bw_sa[w] == -1 && fw_sa[w] != -1)
                        {
//                            q.push(w);
                            q[tail++] = w;
                            if(tail == queue_size)
                                tail = 0;
                            scc_id[w] = 1;
                            bw_sa[w] = level + 1;
                        }
                    }
//                    if(tid == 0)
//                        printf("head, %d; tail, %d\n", head, tail);
                }
//                break;
//                printf("head = %d\n", head);
//                MPI_Allreduce(MPI_IN_PLACE,
//                        bw_sa,
//                        vert_count,
//                        MPI_INT,
//                        MPI_MAX,
//                        MPI_COMM_WORLD);
//
//                MPI_Allreduce(MPI_IN_PLACE,
//                        bw_sa,
//                        vert_count,
//                        MPI_INT,
//                        MPI_MAX,
//                        MPI_COMM_WORLD);

                MPI_Allreduce(MPI_IN_PLACE,
                        scc_id,
                        vert_count,
                        MPI_INT,
                        MPI_MAX,
                        MPI_COMM_WORLD);

//                MPI_Barrier(MPI_COMM_WORLD);
                break;
            }

        front_comm[tid]=front_count;

//        MPI_Barrier(MPI_COMM_WORLD);
        double temp_time = wtime();
        MPI_Allreduce(MPI_IN_PLACE,
                &front_count,
                1,
                MPI_INT,
                MPI_SUM,
                MPI_COMM_WORLD);
        if(is_top_down)
        {
            MPI_Allreduce(MPI_IN_PLACE,
                    &my_work_next,
                    1,
                    MPI_INT,
                    MPI_SUM,
                    MPI_COMM_WORLD);
        }


        if(front_count == 0) 
        {
            MPI_Allreduce(MPI_IN_PLACE,
                    scc_id,
                    vert_count,
                    MPI_INT,
                    MPI_MAX,
                    MPI_COMM_WORLD);
            break;
        }
        sync_time += wtime() - temp_time;

//        #pragma omp barrier
//        front_count=0;
//        my_work_next=0;
//
//        for(index_t i=0;i<thread_count;++i)
//        {
//            front_count += front_comm[i];
//            my_work_next += work_comm[i];
//        }
//            
//        if(front_count == 0) 
//        {
//            break;
//        }
//
//        if(level > 2 || (is_top_down && (my_work_next * alpha > edge_count)))
        if((is_top_down && (my_work_next * alpha > edge_count)))
//        if(is_top_down && my_work_next>(alpha*edge_count)) 
        {
            is_top_down=false;
//            MPI_Allreduce(MPI_IN_PLACE,
//                    bw_sa,
//                    vert_count,
//                    MPI_SIGNED_CHAR,
//                    MPI_MAX,
//                    MPI_COMM_WORLD);

            if(tid==0)
                std::cout<<"--->Switch to bottom up"<<my_work_next
                    <<" "<<edge_count<<"<----\n";
//				if(tid==0)
//				{
//					long todo=0;
//					for(int i=0;i<vert_count;i++)
//						if(bw_sa[i]==-1)
//							todo+=bw_beg_pos[i+1]-bw_beg_pos[i];
//
//					std::cout<<"Edges connected to unvisited: "<<todo<<"\n";
//				}

        }	
        else if(level > 50)
//        else if(!is_top_down && (my_work_next * beta < edge_count) && level % 2 == 0)
//            if(!is_top_down && (my_work_next * beta < edge_count))
            {
                is_top_down_queue = true;

                MPI_Allgather(&bw_sa[vert_beg],
                    step,
                    MPI_SIGNED_CHAR,
                    bw_sa,
                    step,
                    MPI_SIGNED_CHAR,
                    MPI_COMM_WORLD);
                
//                index_t simple_end = world_size * step;
//                if(vert_count != simple_end)
//                {
//    //                    printf("%d, %d\n", simple_end, vert_count);
//                    MPI_Allreduce(MPI_IN_PLACE,
//                        &bw_sa[simple_end],
//                        vert_count - simple_end,
//                        MPI_SIGNED_CHAR,
//                        MPI_MAX,
//                        MPI_COMM_WORLD);
//                }
//				if(tid==0)
//					std::cout<<"--->Switch to top down"<<my_work_next
//						<<" "<<edge_count<<"<----\n";
//				if(tid==0)
//				{
//					long todo=0;
//					for(int i=0;i<vert_count;i++)
//						if(bw_sa[i]==-1)
//							todo+=bw_beg_pos[i+1]-bw_beg_pos[i];
//
//					std::cout<<"Edges connected to unvisited: "<<todo<<"\n";
//				}
            }
        
//        else
//            if(level > 50)
//         if((is_top_down_queue || !is_top_down) && level % 2 == 1)
        if(!is_top_down || is_top_down_queue)
        {

//            MPI_Allreduce(MPI_IN_PLACE,
//                    scc_id,
//                    vert_count,
//                    MPI_INT,
//                    MPI_MAX,
//                    MPI_COMM_WORLD);
            
            double temp_time = wtime();
            if(front_count > 10000)
            {

//                MPI_Allgather(&sa_compress[vert_beg / 32],
//                    step / 32,
//                    MPI_UNSIGNED,
//                    sa_compress,
//                    step / 32,
//                    MPI_UNSIGNED,
//                    MPI_COMM_WORLD);
//
                MPI_Allreduce(MPI_IN_PLACE,
                    sa_compress,
                    //virtual_count / 32,
                    //sa_compress,
                    vert_count / 32 + 32,
                    MPI_UNSIGNED,
                    MPI_BOR,
                    MPI_COMM_WORLD);
                sync_time += wtime() - temp_time;
//                    
                vertex_t num_sa = 0;
                for(index_t i = 0; i < vert_count; ++i)
                {
                    //sa_compress[vert_id >> 5] |= (1 << (vert_id & 31));  
                    if(fw_sa[i] != -1 && bw_sa[i] == -1 && ((sa_compress[i / 32] & ((index_t)1 << ((index_t)i % 32))) != 0))
                    //if(scc_id[v_new] == 0 && fw_sa[v_new] == -1)
                    {
                        bw_sa[i] = level + 1;
                        num_sa += 1;
                    }
                }


//                MPI_Allgather(&bw_sa[vert_beg],
//                    step,
//                    MPI_SIGNED_CHAR,
//                    bw_sa,
//                    step,
//                    MPI_SIGNED_CHAR,
//                    MPI_COMM_WORLD);
                
           }
            else
            {   
                // Step 0: sync front_comm to get the send buffer size of each processor
//                if(tid == 0)
//                {
//                    std::cout<<"sync fq\n";
//                    std::cout<<front_count<<","<<world_rank<<","<<world_size<<"\n";
//                }

                double temp_time = wtime();
//                MPI_Allgather(&front_comm[tid],
//                    1,
//                    MPI_INT,
//                    front_comm,
//                    1,
//                    MPI_INT,
//                    MPI_COMM_WORLD);

                MPI_Allreduce(MPI_IN_PLACE,
                        front_comm,
                        world_size,
                        MPI_INT,
                        MPI_MAX,
                        MPI_COMM_WORLD);

                // Step 1: send fq_comm to other processors
//                    if(tid == 0)
//                        std::cout<<"sending"<<"\n"; 

                MPI_Request request;

                for(int i = 0; i < world_size; ++i)
                {
//                        std::cout<<i<<","<<front_comm[world_rank]<<"\n"; 
                    if(i != world_rank)
                    {
                        MPI_Isend(fq_comm,
                                front_comm[world_rank],
                                MPI_INT,
                                i,
                                0,
                                MPI_COMM_WORLD,
                                &request);
                        MPI_Request_free(&request);
                    }
                }

                // Step 2: recv and store into fq_comm
//                    if(tid == 0)
//                        std::cout<<"receiving"<<"\n"; 
                
                vertex_t fq_begin = front_comm[world_rank];
                for(int i = 0; i < world_size; ++i)
                {
                    if(i != world_rank)
                    {
                        MPI_Recv(&fq_comm[fq_begin],
                                front_comm[i],
                                MPI_INT,
                                i,
                                0,
                                MPI_COMM_WORLD,
                                MPI_STATUS_IGNORE);
                        fq_begin += front_comm[i];
                    }
                }

                sync_time += wtime() - temp_time;

                // Step 3: recover status array fw_sa from the new fq_comm
//                    if(tid == 0)
//                        std::cout<<"recover"<<"\n"; 
                //for(int i = front_comm[world_rank]; i < front_count; ++i)
                for(int i = 0; i < front_count; ++i)
                {
                    vertex_t v_new = fq_comm[i];
                    if(bw_sa[v_new] == -1)
                    {
                        bw_sa[v_new] = level;
                    }
                }

            }
//            MPI_Allreduce(MPI_IN_PLACE,
//                    bw_sa,
//                    vert_count,
//                    MPI_INT,
//                    MPI_MAX,
//                    MPI_COMM_WORLD);
//
//            MPI_Barrier(MPI_COMM_WORLD);
        }

        std::cout<<"BW_Level-"<<(int)level
            <<","<<tid<<","
            <<front_count<<" "
            <<sync_time * 1000<<" ms,"
            <<(wtime() - ltm) * 1000<<" ms "
            <<"\n";
//        if(tid==0)
//            std::cout<<"Level-"<<(int)level<<",time "<<(wtime() - ltm) * 1000<<" ms<----\n";
        level ++;
        sync_time_bw += sync_time;
    }
    std::cout<<"BW comm time,"<<sync_time_bw * 1000<<",(ms)\n";

}

//inline void fw_bfs_fq_queue(
//        index_t *scc_id,
//        index_t *fw_beg_pos,
//        index_t *bw_beg_pos,
//        index_t vert_beg,
//        index_t vert_end,
//        vertex_t *fw_csr,
//        vertex_t *bw_csr,
//        vertex_t *fw_sa,
//        index_t *vertex_cur,
//        index_t *vertex_front,
//        vertex_t root,
//        index_t tid,
//        index_t thread_count,
//        const int alpha,
//        const int beta,
//        const int gamma,
//        vertex_t *frontier_queue,
//        vertex_t fq_size,
//        const double avg_degree,
//        vertex_t vertex_visited,
//        vertex_t *temp_queue,
//        index_t *prefix_sum,
//        vertex_t upper_bound,
//        vertex_t *thread_queue
//        )
//{
//    depth_t level = 0;
//    fw_sa[root] = 0;
//    temp_queue[0] = root;
//    vertex_t queue_size = 1;
////    vertex_t upper_bound = vert_count / thread_count;
////    vertex_t *thread_queue = new vertex_t[upper_bound];
////    vertex_t root_out_degree = fw_beg_pos[root+1] - fw_beg_pos[root];
//    bool is_top_down = true;
////    bool is_top_down_async = false;
////    if(VERBOSE)
////    {
////        if(tid == 0)
////        {
////            printf("out_degree, %d, limit, %.3lf\n", root_out_degree, alpha * beta * fq_size);
////        }
////    }
////    if(root_out_degree < alpha * beta * fq_size)
////    {
////        is_top_down_async = true;
////    }
//    bool is_top_down_queue = false;
////    index_t queue_size = fq_size / thread_count;
//    #pragma omp barrier
//    while(true)
//    {
//        double ltm= wtime();
//        vertex_t vertex_frontier = 0;
//        
////        printf("tid, %d, %d, %d, %d\n", tid, step, queue_beg, queue_end);
//        #pragma omp barrier
//        if(is_top_down)
//        {
//            vertex_t step = queue_size / thread_count;
//            vertex_t queue_beg = tid * step;
//            vertex_t queue_end = (tid == thread_count - 1 ? queue_size: queue_beg + step);
//            for(vertex_t q_vert_id=queue_beg; q_vert_id<queue_end; q_vert_id++)
//            {
//                vertex_t vert_id = temp_queue[q_vert_id];
//                //in fq, scc_id[vert_id] is always not 0
//                if(scc_id[vert_id] == 0 && fw_sa[vert_id] != -1)
//                {
//                    index_t my_beg = fw_beg_pos[vert_id];
//                    index_t my_end = fw_beg_pos[vert_id+1];
//                    for(; my_beg<my_end; my_beg++)
//                    {
//                        vertex_t nebr=fw_csr[my_beg];
//                        if(scc_id[nebr] == 0 && fw_sa[nebr] == -1)
//                        {
//                            fw_sa[nebr] = level+1;
//                            thread_queue[vertex_frontier] = nebr;
//                            vertex_frontier++;
//                        }
//                    }
//                }
//            }
//        }
//        else
//            if(!is_top_down_queue)
//            {
//                for(vertex_t fq_vert_id=vert_beg; fq_vert_id<vert_end; fq_vert_id++)
//                {
//                    vertex_t vert_id = frontier_queue[fq_vert_id];
//                    if(scc_id[vert_id] == 0 && fw_sa[vert_id] == -1)
//                    {
//                        index_t my_beg = bw_beg_pos[vert_id];
//                        index_t my_end = bw_beg_pos[vert_id+1];
////                        my_work_curr+=my_end-my_beg;
//
//                        for(; my_beg<my_end; my_beg++)
//                        {
//                            vertex_t nebr=bw_csr[my_beg];
//                            if(scc_id[nebr] == 0 && fw_sa[nebr] != -1)
////                            if(scc_id[vert_id] == 0 && fw_sa[nebr] == level)
//                            {
//                                fw_sa[vert_id] = level+1;
//                                vertex_frontier++;
////                                front_count++;
//                                break;
//                            }
//                        }
//                    }
//                }
//            }
//            else
//            {
//                vertex_t end_queue = upper_bound;
//                index_t head = 0;
//                index_t tail = 0;
//                //std::queue<index_t> q;
//                vertex_t step = queue_size / thread_count;
//                vertex_t queue_beg = tid * step;
//                vertex_t queue_end = (tid == thread_count - 1 ? queue_size: queue_beg + step);
//
//                //Option 1: put current level vertices into fq
//                for(vertex_t q_vert_id=queue_beg; q_vert_id<queue_end; q_vert_id++)
//                {
//                    thread_queue[tail] = temp_queue[q_vert_id];
//                    tail ++;
//                }
//                while(head != tail)
//                {
//                    vertex_t temp_v = thread_queue[head++];
////                    front_count ++;
//                    if(head == end_queue)
//                        head = 0;
//                    index_t my_beg = fw_beg_pos[temp_v];
//                    index_t my_end = fw_beg_pos[temp_v+1];
//
//                    for(; my_beg < my_end; ++my_beg)
//                    {
//                        index_t w = fw_csr[my_beg];
//                        
//                        if(scc_id[w] == 0 && fw_sa[w] == -1)
//                        {
//                            thread_queue[tail++] = w;
//                            if(tail == end_queue)
//                                tail = 0;
//                            fw_sa[w] = level + 1;
//                        }
//                    }
//                }
//            }
//
//        
//        vertex_front[tid] = vertex_frontier;
//        #pragma omp barrier
//        vertex_frontier = 0;
//
//        for(index_t i=0; i<thread_count; ++i)
//        {
//            vertex_frontier += vertex_front[i];
//        }
//        vertex_visited += vertex_frontier;
////        #pragma omp barrier
//        if(VERBOSE)
//        {
//            double edge_frontier = (double)vertex_frontier * avg_degree;
//            double edge_remaider = (double)(fq_size - vertex_visited) * avg_degree;
//			if(tid==0 && level < 50) 
//                std::cout<<"Level-"<<(int)level<<" "
////				<<"-frontier-time-visited:"
//				<<vertex_frontier<<" "
//                <<fq_size<<" "
//                <<(double)(fq_size)/vertex_frontier<<" "
//				<<(wtime() - ltm) * 1000<<"ms "
//				<<vertex_visited<<" "
//                <<edge_frontier<<" "
//                <<edge_remaider<<" "
//                <<edge_remaider/edge_frontier<<"\n";
//        }
//        
//        if(vertex_frontier == 0) break;
//        
//        if(is_top_down) 
//        {
//            double edge_frontier = (double)vertex_frontier * avg_degree;
//            double edge_remainder = (double)(fq_size - vertex_visited) * avg_degree;
////            printf("edge_remainder/alpha = %g, edge_froniter = %g\n", edge_remainder / alpha, edge_frontier);
//            if(!is_top_down_queue && (edge_remainder / alpha) < edge_frontier)
//            {
//                is_top_down = false;
//                if(VERBOSE)
//                {
//                    if(tid==0)
//                    {
////                        double Nf = vertex_frontier;
////                        double Nu = fq_size - vertex_visited;
////                        double Mf = Nf * Nf / Nu + avg_degree * (Nu - Nf);
////                        printf("mf=%.0lf, mu=%.0lf, alpha=%d, Mf=%.0lf\n", edge_frontier, edge_remainder, ALPHA, Mf);
//                        std::cout<<"--->Switch to bottom up\n";
//                    }
//                }
//            }
//
//        }
//        else
//            if((!is_top_down && !is_top_down_queue && (fq_size*1.0/beta) > vertex_frontier) || (!is_top_down && !is_top_down_queue && level > gamma))
////            if(level > 10)
//            {
//                //if(!is_top_down_queue)
//                
//                vertex_frontier = 0;
//                for(vertex_t fq_vert_id=vert_beg; fq_vert_id<vert_end; fq_vert_id++)
//                {
//                    vertex_t vert_id = frontier_queue[fq_vert_id];
//                    if(scc_id[vert_id] == 0 && fw_sa[vert_id] == level + 1)
//                    {
//                        thread_queue[vertex_frontier] = vert_id;
//                        vertex_frontier++;
//                    }
//                }
//                vertex_front[tid] = vertex_frontier;
//                
//                is_top_down = false;
//                is_top_down_queue = true;
//
//                if(VERBOSE)
//                {
//                    if(tid==0)
//                        std::cout<<"--->Switch to top down queue\n";
//                }
//            }
////                is_top_down = true;
////                is_top_down_queue = true;
////                if(VERBOSE && tid==0)
////                    std::cout<<"--->Switch back to top down\n";
////                vertex_frontier = 0;
////                for(vertex_t fq_vert_id=vert_beg; fq_vert_id<vert_end; fq_vert_id++)
////                {
////                    vertex_t vert_id = frontier_queue[fq_vert_id];
////                    if(scc_id[vert_id] == 0 && fw_sa[vert_id] == level + 1)
////                    {
////                        thread_queue[vertex_frontier] = vert_id;
////                        vertex_frontier++;
////                    }
////                }
////                vertex_front[tid] = vertex_frontier;
////            }
//        
////            if(is_top_down && !is_top_down_queue && (fq_size*1.0/BETA) > vertex_frontier)
//// switch to async top down queue
//
////        if(is_top_down && level > gamma)
////        {
////            // get the frontier queue from bottom up
////            if(!is_top_down_queue)
////            {
////                vertex_frontier = 0;
////                for(vertex_t fq_vert_id=vert_beg; fq_vert_id<vert_end; fq_vert_id++)
////                {
////                    vertex_t vert_id = frontier_queue[fq_vert_id];
////                    if(scc_id[vert_id] == 0 && fw_sa[vert_id] == level + 1)
////                    {
////                        thread_queue[vertex_frontier] = vert_id;
////                        vertex_frontier++;
////                    }
////                }
////                vertex_front[tid] = vertex_frontier;
////            }
////            
////            is_top_down = false;
////            is_top_down_queue = true;
////
////            if(VERBOSE)
////            {
////                if(tid==0)
////                    std::cout<<"--->Switch to top down queue\n";
////            }
////        }
//        
//        #pragma omp barrier
//
//        if(is_top_down || is_top_down_queue)
//        {
//            get_queue(thread_queue,
//                    vertex_front,
//                    prefix_sum,
//                    tid,
//                    temp_queue);
//            queue_size = prefix_sum[thread_count-1] + vertex_front[thread_count-1];
////            if(VERBOSE)
////            {
////                if(tid == 0)
////                    printf("queue_size, %d\n", queue_size);
////            }
//        }
//        #pragma omp barrier
//        
//        level ++;
//    }
////    if(tid == 0)
////    {
////        printf("fw_level, %d\n", level);
////    }
////    delete[] thread_queue;
//}

inline void fw_bfs_fq(
        index_t *scc_id,
        index_t *fw_beg_pos,
        index_t *bw_beg_pos,
        index_t vert_beg,
        index_t vert_end,
        vertex_t *fw_csr,
        vertex_t *bw_csr,
        vertex_t *fw_sa,
        index_t *vertex_cur,
        index_t *vertex_front,
        vertex_t root,
        index_t tid,
        index_t thread_count,
        double alpha,
        double beta,
        vertex_t *frontier_queue,
        vertex_t fq_size,
        const double avg_degree,
        vertex_t vertex_visited
        )
{
    depth_t level = 0;
    fw_sa[root] = 0;
    vertex_t root_out_degree = fw_beg_pos[root+1] - fw_beg_pos[root];
    bool is_top_down = true;
    bool is_top_down_async = false;
    if(VERBOSE)
    {
        if(tid == 0)
        {
            printf("out_degree, %d, limit, %.3lf\n", root_out_degree, alpha * beta * fq_size);
        }
    }
    if(root_out_degree < alpha * beta * fq_size)
    {
        is_top_down_async = true;
    }
    bool is_top_down_queue = false;
    index_t queue_size = fq_size / thread_count;
    #pragma omp barrier
    
    while(true)
    {
//        #pragma omp barrier
        double ltm= wtime();
//        index_t front_count=0;
//        index_t my_work_next=0;
//        index_t my_work_curr=0;
        
        vertex_t vertex_frontier = 0;
//        vertex_t vertex_current = 0;

        if(is_top_down)
        {
            if(is_top_down_async)
            {
                for(vertex_t fq_vert_id=vert_beg; fq_vert_id<vert_end; fq_vert_id++)
                {
                    vertex_t vert_id = frontier_queue[fq_vert_id];
                    //in fq, scc_id[vert_id] is always not 0
                    if(scc_id[vert_id] == 0 && (fw_sa[vert_id]==level || fw_sa[vert_id]==level+1))
                    {
                        index_t my_beg = fw_beg_pos[vert_id];
                        index_t my_end = fw_beg_pos[vert_id+1];

                        for(; my_beg<my_end; my_beg++)
                        {
                            vertex_t nebr=fw_csr[my_beg];
                            if(scc_id[nebr] == 0 && fw_sa[nebr] == -1)
                            {
                                fw_sa[nebr] = level+1;
//                                edge_frontier += fw_beg_pos[nebr+1] - fw_beg_pos[nebr];
                                vertex_frontier ++;
//                                my_work_next+=fw_beg_pos[nebr+1]-fw_beg_pos[nebr];
//
//                                front_count++;
                            }
                        }
                    }
                }
            }
            else
            {
                for(vertex_t fq_vert_id=vert_beg; fq_vert_id<vert_end; fq_vert_id++)
                {
                    vertex_t vert_id = frontier_queue[fq_vert_id];
                    //in fq, scc_id[vert_id] is always not 0
                    if(scc_id[vert_id] == 0 && fw_sa[vert_id]==level)
    //                if(scc_id[vert_id] == 0 && (fw_sa[vert_id]==level || fw_sa[vert_id]==level+1))
                    {
                        index_t my_beg = fw_beg_pos[vert_id];
                        index_t my_end = fw_beg_pos[vert_id+1];

                        for(; my_beg<my_end; my_beg++)
                        {
                            vertex_t nebr=fw_csr[my_beg];
                            if(scc_id[nebr] == 0 && fw_sa[nebr] == -1)
                            {
                                fw_sa[nebr] = level+1;
//                                my_work_next+=fw_beg_pos[nebr+1]-fw_beg_pos[nebr];
//                                front_count++;
                                vertex_frontier++;
                            }
                        }
                    }
                }
            }
//            vertex_front[tid] = vertex_frontier;
        }
        else
            if(!is_top_down_queue)
            {
                for(vertex_t fq_vert_id=vert_beg; fq_vert_id<vert_end; fq_vert_id++)
                {
                    vertex_t vert_id = frontier_queue[fq_vert_id];
                    if(scc_id[vert_id] == 0 && fw_sa[vert_id]== -1)
                    {
                        index_t my_beg = bw_beg_pos[vert_id];
                        index_t my_end = bw_beg_pos[vert_id+1];
//                        my_work_curr+=my_end-my_beg;

                        for(; my_beg<my_end; my_beg++)
                        {
                            vertex_t nebr=bw_csr[my_beg];
                            if(scc_id[vert_id] == 0 && fw_sa[nebr] != -1)
                            {
                                fw_sa[vert_id] = level+1;
                                vertex_frontier++;
//                                front_count++;
                                break;
                            }
                        }
                    }
                }
            }
            else
            {
                index_t *q = new index_t[queue_size];
                index_t head = 0;
                index_t tail = 0;
                //std::queue<index_t> q;

                //Option 1: put current level vertices into fq
                for(vertex_t fq_vert_id=vert_beg; fq_vert_id<vert_end; fq_vert_id++)
                {
                    vertex_t vert_id = frontier_queue[fq_vert_id];
                    if(scc_id[vert_id] == 0 && fw_sa[vert_id] == level)
                    {
                        q[tail++] = vert_id;
//                        front_count ++;
                    }

                }
                while(head != tail)
                {
                    vertex_t temp_v = q[head++];
//                    front_count ++;
                    if(head == queue_size)
                        head = 0;
                    index_t my_beg = fw_beg_pos[temp_v];
                    index_t my_end = fw_beg_pos[temp_v+1];

                    for(; my_beg < my_end; ++my_beg)
                    {
                        index_t w = fw_csr[my_beg];
                        
                        if(scc_id[w] == 0 && fw_sa[w] == -1)
                        {
                            q[tail++] = w;
                            if(tail == queue_size)
                                tail = 0;
                            fw_sa[w] = level + 1;
                        }
                    }
                }
                delete[] q;
//                if(!DEBUG)
//                {
//                    break;
//                }
            }
        
        vertex_front[tid] = vertex_frontier;

        #pragma omp barrier
        vertex_frontier = 0;

        for(index_t i=0; i<thread_count; ++i)
        {
            vertex_frontier += vertex_front[i];
        }
        vertex_visited += vertex_frontier;

        if(VERBOSE)
        {
            double edge_frontier = (double)vertex_frontier * avg_degree;
            double edge_remaider = (double)(fq_size - vertex_visited) * avg_degree;
			if(tid==0) 
                std::cout<<"Level-"<<(int)level<<" "
//				<<"-frontier-time-visited:"
				<<vertex_frontier<<" "
                <<fq_size<<" "
                <<(double)(fq_size)/vertex_frontier<<" "
				<<(wtime() - ltm) * 1000<<"ms "
				<<vertex_visited<<" "
                <<edge_frontier<<" "
                <<edge_remaider<<" "
                <<edge_remaider/edge_frontier<<"\n";
        }
        if(vertex_frontier == 0) break;
        
        #pragma omp barrier
        
        if(is_top_down) 
        {
            double edge_frontier = (double)vertex_frontier * avg_degree;
            double edge_remainder = (double)(fq_size - vertex_visited) * avg_degree;
            if((edge_remainder / alpha) < edge_frontier)
            {
                is_top_down = false;
                if(VERBOSE)
                {
                    if(tid==0)
                        std::cout<<"--->Switch to bottom up\n";
                }
            }

        }
        else
            if(!is_top_down && !is_top_down_queue && (fq_size*1.0/beta) > vertex_frontier)
            {
                is_top_down_queue = true;
                if(VERBOSE)
                {
                    if(tid==0)
                        std::cout<<"--->Switch to top down queue\n";
                }
            }
        #pragma omp barrier
        level ++;
    }
}

//inline void bw_bfs_fq_queue(
//        index_t *scc_id,
//        index_t *fw_beg_pos,
//        index_t *bw_beg_pos,
//        index_t vert_beg,
//        index_t vert_end,
//        vertex_t *fw_csr,
//        vertex_t *bw_csr,
//        vertex_t *fw_sa,
//        vertex_t *bw_sa,
//        index_t *vertex_cur,
//        index_t *vertex_front,
//        vertex_t root,
//        index_t tid,
//        index_t thread_count,
//        const int alpha,
//        const int beta,
//        const int gamma,
//        vertex_t *frontier_queue,
//        vertex_t fq_size,
//        const double avg_degree,
//        vertex_t vertex_visited,
//        vertex_t *temp_queue,
//        index_t *prefix_sum,
//        vertex_t upper_bound,
//        vertex_t *thread_queue
//        )
//{
//    depth_t level = 0;
//    bw_sa[root] = 0;
//    scc_id[root] = 1;
//    temp_queue[0] = root;
//    vertex_t queue_size = 1;
////    vertex_t upper_bound = vert_count/thread_count * 10;
////    vertex_t *thread_queue = new vertex_t[upper_bound];
//    if(VERBOSE)
//    {
//        if(tid == 0)
//        {
//            printf("upperbound, %d\n", upper_bound);
//        }
//    }
////    vertex_t root_out_degree = fw_beg_pos[root+1] - fw_beg_pos[root];
//    bool is_top_down = true;
////    bool is_top_down_async = false;
////    if(VERBOSE)
////    {
////        if(tid == 0)
////        {
////            printf("out_degree, %d, limit, %.3lf\n", root_out_degree, alpha * beta * fq_size);
////        }
////    }
////    if(root_out_degree < alpha * beta * fq_size)
////    {
////        is_top_down_async = true;
////    }
//    bool is_top_down_queue = false;
////    index_t queue_size = fq_size / thread_count;
//    #pragma omp barrier
//    while(true)
//    {
//        double ltm= wtime();
//        vertex_t vertex_frontier = 0;
//        
//        #pragma omp barrier
//        if(is_top_down)
//        {
//            vertex_t step = queue_size / thread_count;
//            vertex_t queue_beg = tid * step;
//            vertex_t queue_end = (tid == thread_count - 1 ? queue_size: queue_beg + step);
//            
////            printf("tid, %d, %d, %d, %d\n", tid, step, queue_beg, queue_end);
//            for(vertex_t q_vert_id=queue_beg; q_vert_id<queue_end; q_vert_id++)
//            {
//                vertex_t vert_id = temp_queue[q_vert_id];
////                printf("vert_id, %d\n", vert_id);
//                //in fq, scc_id[vert_id] is always not 0
//                if(scc_id[vert_id] == 1)// && bw_sa[vert_id] == level)
//                {
//                    index_t my_beg = bw_beg_pos[vert_id];
//                    index_t my_end = bw_beg_pos[vert_id+1];
////                    printf("my_beg, %d, my_end, %d\n", my_beg, my_end);
//                    for(; my_beg<my_end; my_beg++)
//                    {
//                        vertex_t nebr=bw_csr[my_beg];
//                        if(scc_id[nebr] == 0 && bw_sa[nebr] == -1 && fw_sa[nebr] != -1)
//                        {
//                            bw_sa[nebr] = level+1;
//                            thread_queue[vertex_frontier] = nebr;
//                            vertex_frontier++;
//                            scc_id[nebr] = 1;
//                        }
//                    }
//                }
//            }
//        }
//        else
//            if(!is_top_down_queue)
//            {
//                for(vertex_t fq_vert_id=vert_beg; fq_vert_id<vert_end; fq_vert_id++)
//                {
//                    vertex_t vert_id = frontier_queue[fq_vert_id];
//                    if(scc_id[vert_id] == 0 && fw_sa[vert_id] != -1)
//                    {
//                        index_t my_beg = fw_beg_pos[vert_id];
//                        index_t my_end = fw_beg_pos[vert_id+1];
////                        my_work_curr+=my_end-my_beg;
//
//                        for(; my_beg<my_end; my_beg++)
//                        {
//                            vertex_t nebr=fw_csr[my_beg];
//                            if(scc_id[nebr] == 1)
////                            if(scc_id[nebr] == 1 && bw_sa[nebr] == level)
//                            {
//                                bw_sa[vert_id] = level+1;
//                                vertex_frontier++;
//                                scc_id[vert_id] = 1;
////                                front_count++;
//                                break;
//                            }
//                        }
//                    }
//                }
//            }
//            else
//            {
//                vertex_t end_queue = upper_bound;
//                index_t head = 0;
//                index_t tail = 0;
//                //std::queue<index_t> q;
//                vertex_t step = queue_size / thread_count;
//                vertex_t queue_beg = tid * step;
//                vertex_t queue_end = (tid == thread_count - 1 ? queue_size: queue_beg + step);
//
//                //Option 1: put current level vertices into fq
//                for(vertex_t q_vert_id=queue_beg; q_vert_id<queue_end; q_vert_id++)
//                {
//                    thread_queue[tail] = temp_queue[q_vert_id];
//                    tail ++;
//                }
//                while(head != tail)
//                {
//                    vertex_t temp_v = thread_queue[head++];
////                    front_count ++;
//                    if(head == end_queue)
//                        head = 0;
//                    index_t my_beg = bw_beg_pos[temp_v];
//                    index_t my_end = bw_beg_pos[temp_v+1];
//
//                    for(; my_beg < my_end; ++my_beg)
//                    {
//                        index_t w = bw_csr[my_beg];
//                        
//                        if(scc_id[w] == 0 && bw_sa[w] == -1 && fw_sa[w] != -1)
//                        {
//                            thread_queue[tail++] = w;
//                            if(tail == end_queue)
//                                tail = 0;
//                            scc_id[w] = 1; 
//                            bw_sa[w] = level + 1;
//                        }
//                    }
//                }
//            }
//
//        
//        vertex_front[tid] = vertex_frontier;
//        #pragma omp barrier
//        vertex_frontier = 0;
//
//        for(index_t i=0; i<thread_count; ++i)
//        {
//            vertex_frontier += vertex_front[i];
//        }
//        vertex_visited += vertex_frontier;
////        #pragma omp barrier
//        if(VERBOSE)
//        {
//            double edge_frontier = (double)vertex_frontier * avg_degree;
//            double edge_remaider = (double)(fq_size - vertex_visited) * avg_degree;
//			if(tid==0 && level < 50) 
//                std::cout<<"Level-"<<(int)level<<" "
////				<<"-frontier-time-visited:"
//				<<vertex_frontier<<" "
//                <<fq_size<<" "
//                <<(double)(fq_size)/vertex_frontier<<" "
//				<<(wtime() - ltm) * 1000<<"ms "
//				<<vertex_visited<<" "
//                <<edge_frontier<<" "
//                <<edge_remaider<<" "
//                <<edge_remaider/edge_frontier<<"\n";
//        }
//        
//        if(vertex_frontier == 0) break;
//        
//        if(is_top_down) 
//        {
//            double edge_frontier = (double)vertex_frontier * avg_degree;
//            double edge_remainder = (double)(fq_size - vertex_visited) * avg_degree;
//            if(!is_top_down_queue && (edge_remainder / alpha) < edge_frontier)
//            {
//                is_top_down = false;
//                if(VERBOSE)
//                {
//                    if(tid==0)
//                        std::cout<<"--->Switch to bottom up\n";
//                }
//            }
//
//        }
//        else
//            if((!is_top_down && !is_top_down_queue && (fq_size*1.0/beta) > vertex_frontier) || (!is_top_down && !is_top_down_queue && level > gamma))
//                    //            if(level > 10)
//            {
//                vertex_frontier = 0;
//                for(vertex_t fq_vert_id=vert_beg; fq_vert_id<vert_end; fq_vert_id++)
//                {
//                    vertex_t vert_id = frontier_queue[fq_vert_id];
//                    if(scc_id[vert_id] == 1 && bw_sa[vert_id] == level + 1)
//                    {
//                        thread_queue[vertex_frontier] = vert_id;
//                        vertex_frontier++;
//                    }
//                }
//                vertex_front[tid] = vertex_frontier;
//           
//
//                is_top_down = false;
//                is_top_down_queue = true;
//                if(VERBOSE)
//                {
//                    if(tid==0)
//                        std::cout<<"--->Switch to top down queue\n";
//                }
//            }
////                is_top_down = true;
////                is_top_down_queue = true;
////                if(VERBOSE && tid==0)
////                    std::cout<<"--->Switch back to top down\n";
////                vertex_frontier = 0;
////                for(vertex_t fq_vert_id=vert_beg; fq_vert_id<vert_end; fq_vert_id++)
////                {
////                    vertex_t vert_id = frontier_queue[fq_vert_id];
////                    if(scc_id[vert_id] == 1 && bw_sa[vert_id] == level + 1)
////                    {
////                        thread_queue[vertex_frontier] = vert_id;
////                        vertex_frontier++;
////                    }
////                }
////                vertex_front[tid] = vertex_frontier;
////            }
//        
////            if(is_top_down && !is_top_down_queue && (fq_size*1.0/BETA) > vertex_frontier)
//// switch to async top down queue
//
////        if(is_top_down && level > gamma)
////        {
////            if(!is_top_down_queue)
////            {
////                vertex_frontier = 0;
////                for(vertex_t fq_vert_id=vert_beg; fq_vert_id<vert_end; fq_vert_id++)
////                {
////                    vertex_t vert_id = frontier_queue[fq_vert_id];
////                    if(scc_id[vert_id] == 1 && bw_sa[vert_id] == level + 1)
////                    {
////                        thread_queue[vertex_frontier] = vert_id;
////                        vertex_frontier++;
////                    }
////                }
////                vertex_front[tid] = vertex_frontier;
////            }
////
////            is_top_down = false;
////            is_top_down_queue = true;
////            if(VERBOSE)
////            {
////                if(tid==0)
////                    std::cout<<"--->Switch to top down queue\n";
////            }
////        }
//        
//        #pragma omp barrier
//
//        if(is_top_down || is_top_down_queue)
//        {
//            get_queue(thread_queue,
//                    vertex_front,
//                    prefix_sum,
//                    tid,
//                    temp_queue);
//            queue_size = prefix_sum[thread_count-1] + vertex_front[thread_count-1];
////            if(VERBOSE && tid == 0)
////            {
////                printf("queue_size, %d\n", queue_size);
////            }
//        }
//
//        #pragma omp barrier
//        
//        level ++;
//    }
////    if(tid == 0)
////        printf("bw_level, %d\n", level);
////    delete[] thread_queue;
//}

inline void bw_bfs_fq(
        index_t *scc_id,
        index_t *fw_beg_pos,
        index_t *bw_beg_pos,
        index_t vert_beg,
        index_t vert_end,
        vertex_t *fw_csr,
        vertex_t *bw_csr,
        vertex_t *fw_sa,
        vertex_t *bw_sa,
        index_t *vertex_cur,
        index_t *vertex_front,
        vertex_t root,
        index_t tid,
        index_t thread_count,
        double alpha,
        double beta,
        vertex_t *frontier_queue,
        vertex_t fq_size,
        const double avg_degree,
        vertex_t vertex_visited
        )
{
    bw_sa[root] = 0;
    vertex_t root_in_degree = bw_beg_pos[root+1] - bw_beg_pos[root];
    bool is_top_down = true;
    bool is_top_down_queue = false;
    bool is_top_down_async = false;
    if(DEBUG)
    {
        if(tid == 0)
        {
            printf("in_degree, %d, limit, %.3lf\n", root_in_degree, alpha * beta * fq_size);
        }
    }
    if(root_in_degree < alpha * beta * fq_size)
    {
        is_top_down_async = true;
    }
    index_t level = 0;
    scc_id[root] = 1;
    index_t queue_size = fq_size / thread_count;
    while(true)
    {
        double ltm= wtime();
//        index_t front_count=0;
//        index_t my_work_next=0;
//        index_t my_work_curr=0;
        vertex_t vertex_frontier = 0;

        if(is_top_down)
        {
            if(is_top_down_async)
            {
                for(vertex_t fq_vert_id=vert_beg; fq_vert_id<vert_end; fq_vert_id++)
                {
                    vertex_t vert_id = frontier_queue[fq_vert_id];
                    if(scc_id[vert_id] == 1 && (bw_sa[vert_id] == level || bw_sa[vert_id] == level + 1))
                    {
                        index_t my_beg = bw_beg_pos[vert_id];
                        index_t my_end = bw_beg_pos[vert_id+1];

    //                        printf("my_beg = %d, my_end = %d\n", my_beg, my_end);
                        for(; my_beg<my_end; my_beg++)
                        {
                            vertex_t nebr=bw_csr[my_beg];
                            if(scc_id[nebr] == 0 && bw_sa[nebr] == -1 && fw_sa[nebr] != -1)
                            {
                                bw_sa[nebr] = level+1;
//                                my_work_next+=bw_beg_pos[nebr+1]-bw_beg_pos[nebr];
                                vertex_frontier++;
                                scc_id[nebr] = 1;	
                            }
                        }
                    }
                }
            }
            else
            {
                for(vertex_t fq_vert_id=vert_beg; fq_vert_id<vert_end; fq_vert_id++)
                {
                    vertex_t vert_id = frontier_queue[fq_vert_id];
                    if(scc_id[vert_id] == 1 && bw_sa[vert_id] == level)
                    {
                        index_t my_beg = bw_beg_pos[vert_id];
                        index_t my_end = bw_beg_pos[vert_id+1];

    //                        printf("my_beg = %d, my_end = %d\n", my_beg, my_end);
                        for(; my_beg<my_end; my_beg++)
                        {
                            vertex_t nebr=bw_csr[my_beg];
                            if(scc_id[nebr] == 0 && bw_sa[nebr] == -1 && fw_sa[nebr] != -1)
                            {
                                bw_sa[nebr] = level+1;
//                                my_work_next+=bw_beg_pos[nebr+1]-bw_beg_pos[nebr];
                                vertex_frontier++;
//                                front_count++;
                                scc_id[nebr] = 1;	
                            }
                        }
                    }
                }

            }
//            work_comm[tid]=my_work_next;
        }
        else
            if(!is_top_down_queue)
            {
                for(vertex_t fq_vert_id=vert_beg; fq_vert_id<vert_end; fq_vert_id++)
                {
                    vertex_t vert_id = frontier_queue[fq_vert_id];
//                    if(scc_id[vert_id] == 0 && bw_sa[vert_id] == -1 && fw_sa[vert_id] != -1)
                    if(scc_id[vert_id] == 0)
                    {
                        index_t my_beg = fw_beg_pos[vert_id];
                        index_t my_end = fw_beg_pos[vert_id+1];
//                        my_work_curr+=my_end-my_beg;

                        for(; my_beg<my_end; my_beg++)
                        {
                            vertex_t nebr=fw_csr[my_beg];
//                            if(scc_id[nebr] == 1 && bw_sa[nebr] != -1 && fw_sa[nebr] != -1)
                            if(scc_id[nebr] == 1)
                            {
                                bw_sa[vert_id] = level+1;
//                                front_count++;
                                vertex_frontier++;
                                scc_id[vert_id] = 1;
                                break;
                            }
                        }
                    }
                }
//                work_comm[tid]=my_work_curr;
            }
            else
            {
//                std::queue<index_t> q;
                index_t *q = new index_t[queue_size];
                index_t head = 0;
                index_t tail = 0;
                
                for(vertex_t fq_vert_id=vert_beg; fq_vert_id<vert_end; fq_vert_id++)
                {
                    vertex_t vert_id = frontier_queue[fq_vert_id];
                    if(scc_id[vert_id] == 1 && bw_sa[vert_id] == level)
                    {
                        q[tail++] = vert_id;
//                        front_count ++;
                        //impossible here
//                        if(tail == queue_size)
//                            tail = 0;
                    }
                }

                while(head != tail)
                {
                    vertex_t temp_v = q[head++];
//                    front_count ++;
                    if(head == queue_size)
                        head = 0;
                    index_t my_beg = bw_beg_pos[temp_v];
                    index_t my_end = bw_beg_pos[temp_v+1];

                    for(; my_beg < my_end; ++my_beg)
                    {
                        index_t w = bw_csr[my_beg];
                        
                        if(scc_id[w] == 0 && bw_sa[w] == -1 && fw_sa[w] != -1)
                        {
                            q[tail++] = w;
                            if(tail == queue_size)
                                tail = 0;
                            scc_id[w] = 1;
                            bw_sa[w] = level + 1;
                        }
                    }
                }
                delete[] q;
                if(!DEBUG)
                    break;

            }

//        front_comm[tid]=front_count;

        vertex_front[tid] = vertex_frontier;

        #pragma omp barrier
        vertex_frontier = 0;

        for(index_t i=0; i<thread_count; ++i)
        {
            vertex_frontier += vertex_front[i];
        }
        vertex_visited += vertex_frontier;

        if(VERBOSE)
        {
            double edge_frontier = (double)vertex_frontier * avg_degree;
            double edge_remaider = (double)(fq_size - vertex_visited) * avg_degree;
			if(tid==0) 
                std::cout<<"Level-"<<(int)level<<" "
//				<<"-frontier-time-visited:"
				<<vertex_frontier<<" "
                <<fq_size<<" "
                <<(double)(fq_size)/vertex_frontier<<" "
				<<(wtime() - ltm) * 1000<<"ms "
				<<vertex_visited<<" "
                <<edge_frontier<<" "
                <<edge_remaider<<" "
                <<edge_remaider/edge_frontier<<"\n";
        }
        if(vertex_frontier == 0) break;
        
        #pragma omp barrier
        
        if(is_top_down) 
        {
            double edge_frontier = (double)vertex_frontier * avg_degree;
            double edge_remainder = (double)(fq_size - vertex_visited) * avg_degree;
            if((edge_remainder / alpha) < edge_frontier)
            {
                is_top_down = false;
                if(VERBOSE)
                {
                    if(tid==0)
                        std::cout<<"--->Switch to bottom up\n";
                }
            }

        }
        else
            if(!is_top_down && !is_top_down_queue && (fq_size*1.0/beta) > vertex_frontier)
            {
                is_top_down_queue = true;
                if(VERBOSE)
                {
                    if(tid==0)
                        std::cout<<"--->Switch to top down queue\n";
                }
            }
        #pragma omp barrier
        level ++;
/*        #pragma omp barrier
        front_count=0;
        my_work_next=0;

        for(index_t i=0;i<thread_count;++i)
        {
            front_count += front_comm[i];
            my_work_next += work_comm[i];
        }

        if(DEBUG)
        {
            if(tid==0) 
                std::cout<<"Level-"<<(int)level
                <<"-frontier-time-futurework(currwork in btup): "
                <<front_count<<" "
                <<(wtime() - ltm) * 1000<<" ms "
                <<my_work_next<<"\n";
        }
            
        if(front_count == 0) 
        {
            break;
        }
        
        if(is_top_down && my_work_next > (alpha*fq_size)) 
        {
            is_top_down=false;
            if(DEBUG)
            {
                if(tid==0)
                    std::cout<<"--->Switch to bottom up"<<my_work_next<<" "<<fq_size<<"<----\n";
            }
        }
        else
            if(!is_top_down && !is_top_down_queue && front_count < (beta * fq_size))
            {
                is_top_down_queue = true;
                if(DEBUG)
                {
                    if(tid==0)
                        std::cout<<"--->Switch to top down queue"<<my_work_next<<" "<<fq_size<<"<----\n";
                }
            }
        #pragma omp barrier

        level ++;
        */
    }
    if(tid == 0)
        printf("bw_level, %d\n", level);
}

inline void process_wcc(
        index_t vert_beg,
        index_t vert_end,
        vertex_t *wcc_fq,
        vertex_t *color,
        vertex_t &wcc_fq_size
        )
{
    std::set<int> s_fq;
    for(vertex_t i = vert_beg; i < vert_end; ++i)
    {
//            vertex_t v = frontier_queue[i];
//            vertex_t wcc_id = color[v];
        if(s_fq.find(i) == s_fq.end()) 
            s_fq.insert(color[i]);
    }

    wcc_fq_size = s_fq.size();
    std::set<int>::iterator it;
    int i=0;
    for(it = s_fq.begin(); it != s_fq.end(); ++it, ++i)
    {
//            printf("%d", *it);
        wcc_fq[i] = *it;
    }
}

inline void mice_fw_bw(
        color_t *wcc_color,
        index_t *scc_id,
        index_t *sub_fw_beg,
        index_t *sub_bw_beg,
        vertex_t *sub_fw_csr,
        vertex_t *sub_bw_csr,
        vertex_t *fw_sa,
        index_t tid,
        index_t thread_count,
        vertex_t *frontier_queue,
        vertex_t sub_v_count,
        vertex_t *wcc_fq,
        vertex_t wcc_fq_size
        )
{
//    thread_count = 1;
    index_t step = wcc_fq_size / thread_count;
    index_t wcc_beg = tid * step;
    index_t wcc_end = (tid == thread_count - 1 ? wcc_fq_size : wcc_beg + step);

//    index_t wcc_beg = 0;
//    index_t wcc_end = wcc_fq_size;
//    
    index_t *q = new index_t[sub_v_count];
    index_t head = 0;
    index_t tail = 0;

    for(vertex_t v = 0; v < sub_v_count; ++v)
    {
        if(scc_id[v] == -1)
        {
            vertex_t cur_wcc = wcc_color[v];
            bool in_wcc = false;
            for(vertex_t i = wcc_beg; i < wcc_end; ++i)
            {
                if(wcc_fq[i] == cur_wcc)
                {
                    in_wcc = true;
                    break;
                }
            }
            if(in_wcc)
            {
//                if(tid == 0)
//                {
            //        printf("v, %d, wcc, %d\n", v, wcc_color[v]);
//                }
                //fw
                fw_sa[v] = v;
                q[tail++] = v;
                if(tail == sub_v_count)
                    tail = 0;
           //     std::cout<<"1, tail,"<<tail<<"\n";
                while(head != tail)
                {
                    vertex_t temp_v = q[head++];
    //                    front_count ++;
                    if(head == sub_v_count)
                        head = 0;
                    index_t my_beg = sub_fw_beg[temp_v];
                    index_t my_end = sub_fw_beg[temp_v + 1];

                    for(; my_beg < my_end; ++my_beg)
                    {
                        index_t w = sub_fw_csr[my_beg];
                        
                        if(fw_sa[w] != v)
                        {
                            q[tail++] = w;
                            if(tail == sub_v_count)
                                tail = 0;
                            fw_sa[w] = v;
                        }
                    }
                }
          //      std::cout<<"2, tail,"<<tail<<"\n";
                //bw
                scc_id[v] = v;
                q[tail++] = v;
                if(tail == sub_v_count)
                    tail = 0;

                while(head != tail)
                {
                    vertex_t temp_v = q[head++];
         //           std::cout<<"tail,"<<tail<<",head,"<<head<<",temp_v,"<<temp_v<<"\n";
    //                    front_count ++;
                    if(head == sub_v_count)
                        head = 0;
                    index_t my_beg = sub_bw_beg[temp_v];
                    index_t my_end = sub_bw_beg[temp_v+1];

                    for(; my_beg < my_end; ++my_beg)
                    {
                        index_t w = sub_bw_csr[my_beg];
                        
                        if(scc_id[w] == -1 && fw_sa[w] == v)
                        {
                            q[tail++] = w;
                            if(tail == sub_v_count)
                                tail = 0;
                            scc_id[w] = v;
                        }
                    }
                }
        //        std::cout<<"3, tail,"<<tail<<"\n";
                
            }
            
        }
    }
    delete[] q;
}
#endif
