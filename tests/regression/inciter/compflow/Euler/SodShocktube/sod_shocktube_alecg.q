# vim: filetype=sh:
# This is a comment
# Keywords are case-sensitive

title "Sod shock-tube"

inciter

  nstep 10    # Max number of time steps
  term 0.2    # Max physical time
  ttyi 1      # TTY output interval
  cfl 0.5

  scheme alecg

  partitioning
    algorithm mj
  end

  compflow
    depvar u
    physics euler
    problem sod_shocktube

    material
      gamma 1.4 end
    end

    bc_sym
      sideset 2 4 5 6 end
    end
  end

  plotvar
    interval 10000
  end

  diagnostics
    interval  1
    format    scientific
    error l2
  end

  history
    point p1 0.1 0.05 0.025 end
    point p2 0.9 0.05 0.025 end
  end

end
