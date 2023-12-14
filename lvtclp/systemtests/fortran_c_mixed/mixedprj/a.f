      subroutine cal2(A, B, C)
      real*8 A,B,C
      C = B - A
      if (A .gt. 0) then
      call cal1(A - 1, B, C)
      endif
      end subroutine cal2

      subroutine cal1(A, B, C)
      real*8 A,B,C
100   call cal2(A, B, C)
102   call cal3(A, B, C)
      return
      end subroutine cal1

      INCLUDE 'b.f'
