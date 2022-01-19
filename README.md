# b-tree_smr
# 用法
輸入一個trace file，然後輸出smr trace,ssd trace, hybrid trace
## input格式為: W/R address data_size
R 56265888 16
## output格式為: time device  address data_size W/R
0.1	1 	56265888	  16	  0
# code
## block
 紀錄block的資訊
  ### value: data的index
  ### data_length: data的大小
  ### tNode: 當前b+tree的leaf中node的數量
  ### crossing_zone: 記錄跨zone的data
  
## node
 用來建立linked list，用來連接zone
  
  
## spiltleaf
  當tNode滿了就做spilt
  
## merge
 將leaf合併
## searchNode
  找有無相同的index，有的話做updateNode，反之做insertNode
  ### updateNode
    block的資料更新，也更新zone中invalid的個數
  ### insertNode
    把新的index放入b+tree中

## gc_invild_zone
 挑出最多invaild的zone去做gc，同時把要gc的zone中valid的資料copy到free_zone中
  

  

  
  
