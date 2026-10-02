# p-adic

Small interactive pictures for p-adic/Witt-vector ideas.

## Witt-vector ring sketch

Open `index.html` in a browser.

The current sketch uses five concentric coordinate rings. For a chosen prime
`p = 2, 3, 5`, ring `i` has `p` sectors representing the sampled Witt
coordinate `x_i ∈ {0,…,p−1}`.

Interaction:

- click a sector to change `x_i`;
- the yellow polyline is the selected coordinate path;
- changing `x_i` sends blue ripples through levels `i,i+1,…`;
- the panel evaluates the p-typical ghost polynomials
  `w_n = x_0^(p^n) + p x_1^(p^(n-1)) + … + p^n x_n` in the integers;
- randomize, zero, and Teichmüller buttons give quick contrasting states.

This is deliberately a visual experiment rather than a claim that concentric
rings are a canonical picture of Witt vectors. The point is to test whether
coordinate depth, p-way branching, and triangular ghost dependence can be made
visible in one object.
