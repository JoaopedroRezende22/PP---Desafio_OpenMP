# Teste de escalabilidade: roda a imagem cv-openmp com 1, 2, 4, 8 e 12 threads.
# Cada configuracao e repetida varias vezes e usa a MEDIANA, para reduzir o ruido.
#
# Uso (PowerShell, dentro da pasta desafio_openmp):
#   .\benchmark.ps1                  -> 100000 repeticoes, 3 rodadas por configuracao
#   .\benchmark.ps1 -Rodadas 5       -> 5 rodadas por configuracao
#   .\benchmark.ps1 -Threads 1,2,4   -> so algumas configuracoes

param(
    [int]$Reps = 100000,
    [int]$Rodadas = 3,
    [string]$Imagem = "cv-openmp",
    [int[]]$Threads = @(1, 2, 4, 8, 12)
)

$cultura = [System.Globalization.CultureInfo]::InvariantCulture

function Mediana($valores) {
    $s = @($valores | Sort-Object)
    $n = $s.Count
    if ($n % 2 -eq 1) { return $s[[int](($n - 1) / 2)] }
    return ($s[[int]($n / 2) - 1] + $s[[int]($n / 2)]) / 2
}

$linhas = @()

foreach ($t in $Threads) {
    $seqs = @()
    $pars = @()

    for ($r = 1; $r -le $Rodadas; $r++) {
        Write-Host ("Threads={0} rodada {1}/{2} ..." -f $t, $r, $Rodadas)
        $saida = docker run --rm --cpus=$t $Imagem ./cv_openmp dados.txt $t $Reps

        foreach ($l in $saida) {
            if ($l -match '^Sequencial\s*:\s*([0-9.]+)') {
                $seqs += [double]::Parse($matches[1], $cultura)
            }
            elseif ($l -match '^Paralelo\s*:\s*([0-9.]+)') {
                $pars += [double]::Parse($matches[1], $cultura)
            }
        }
    }

    if ($seqs.Count -eq 0 -or $pars.Count -eq 0) {
        Write-Host "Nao foi possivel ler os tempos para $t threads. Verifique se a imagem $Imagem existe." -ForegroundColor Red
        continue
    }

    $seqMed = Mediana $seqs
    $parMed = Mediana $pars
    $speedup = $seqMed / $parMed
    $eficiencia = 100.0 * $speedup / $t

    $linhas += [pscustomobject]@{
        Threads        = $t
        Seq_s          = [math]::Round($seqMed, 4)
        Par_s          = [math]::Round($parMed, 4)
        Speedup        = [math]::Round($speedup, 2)
        Eficiencia_pct = [math]::Round($eficiencia, 1)
        Rodadas        = $Rodadas
        Repeticoes     = $Reps
    }
}

$linhas | Format-Table -AutoSize
$linhas | Export-Csv -Path "resultados_threads.csv" -NoTypeInformation -Encoding UTF8
Write-Host "Resultados salvos em resultados_threads.csv"
