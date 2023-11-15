      subroutine cal2(A, B, C)
      real*8 A,B,C
      C = B - A
      end subroutine cal2

      subroutine cal1(A, B, C)
      real*8 A,B,C
      if (A .gt. 0) then
      call calme(A - 1, B, C)
      endif
100   call cal2(A, B, C)
      A = cal_f(A)
      return
      end subroutine cal1

      INCLUDE 'b.f'

      function cal_f(A)
      real*8 A
      A = 1
      end function cal_f
