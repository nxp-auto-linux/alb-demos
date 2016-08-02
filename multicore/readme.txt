This C sample illustrates the benefits of multicore processing.
It is known that the multiplication algoritm of a bidimensional 
array has a O(n^3) time complexity because of it's three 'for' 
structures. To push this case to the limits, we simultaneously 
take five different bidimensional arrays from five different 
files in order to multiply them 1923 times.

Firstly, we process things in sequence and we put two timestamps
at the beginning and at the end of this process in order to 
determine the time spent by the processor to complete the task.
After that, we use threads to do exactly the same thing and we 
also measure the time spent by the processor to complete the task.

The advantage of having a multicore processor is that you can break 
a major task into pieces and solve them simultaneously. The only 
"rule" is not to interfere sequence at the same time because one of 
it can need the output from another and it will fail without a correct
set of entry.

We considered that our virtual machine has a processor with 4 cores. 
Cores 0 to 3 have assigned the first four threads while the fifth one 
will go on the first core that will become free (almost a random choice).
In "functions.c" we have the functions that will be used. 


-void make_matrix(char path[], int matrix[max_size][max_size]) is a 
  function that takes  from the files some integer values and makes 
  a bidimensional array with it. This function receives as parameters 
  the path of the file and the resulting matrix. It doesn't return any
  kind of value.

-void display_matrix(int matrix[max_size][max_size], int size) deals
  with displaying a matrix whose name and size will be the parameters 
  of this function

-void multi_matrix(int matrix[max_size][max_size], int size) deals with
  multiplying a matrix with a given size

-void* thread_multi_matrix(void *params) is the multi_matrix() alternative
  for threads implementation; *params are given through a structure that
  contains the size of the matrix, the matrix itself and the id of the 
  thread associate.




            --------------------------------------------     
            -                                          -      
            -                Start process             -     
            -                                          -  
            --------------------------------------------  
                   /       /     |       \       \                  
                 /        /      |        \        \               
               /         /       |         \         \              
             /          /        |          \          \           
           /           /         |           \           \        
         /            /          |            \            \     
       /             /           |             \             \     
     /              /            |              \              \     
-----------   -----------   -----------   -----------   -----------
-         -   -         -   -         -   -         -   -         - 
-Thread 0 -   -Thread 1 -   -Thread 2 -   -Thread 3 -   -Thread 4 -
-         -   -         -   -         -   -         -   -         -
-----------   -----------   -----------   -----------   -----------
     |             |             |             |              |  
     |             |             |             |              |  
     |             |             |             |              |  
     |             |             |             |              |  
     |             |             |             |              |  
------------  ------------   ------------   ------------   ----------------
-          -  -          -   -          -   -          -   -   Random     - 
-  Core 0  -  -  Core 1  -   -  Core 2  -   -  Core 3  -   -Core {0,1,2,3}-
-          -  -          -   -          -   -          -   -   Random     -
------------  ------------   ------------   ------------   ----------------
    \               \              |              /               / 
      \              \             |             /              /    
        \             \            |            /             /       
          \            \           |           /            /        
            \           \          |          /           /         
              \          \         |         /          /          
                \         \        |        /         /              
                  \        \       |       /        / 
            --------------------------------------------     
            -                                          -      
            -                Join Threads              -     
            -                                          -  
            --------------------------------------------  
                                  |
                                  |
                                  |
            --------------------------------------------     
            -                                          -      
            -                End process               -     
            -                                          -  
            --------------------------------------------
