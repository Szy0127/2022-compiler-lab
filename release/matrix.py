A = []
B = []
C = []
N = 28
for i in range(N):
    A.append([])
    B.append([])
    C.append([])
    for j in range(N):
        A[i].append([])
        B[i].append([])
        C[i].append([])
        A[i][j] = i * N + j
        B[i][j] = j * N + i
        C[i][j] = 0
for i in range(N):
    for j in range(N):
        for k in range(N):
            C[i][j] += A[i][k] + B[k][j]
         
print(C[0][0]+C[10][8]+C[9][20])