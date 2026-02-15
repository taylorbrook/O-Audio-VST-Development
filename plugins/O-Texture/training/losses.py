"""
O-Texture Loss Functions
Multi-scale spectral loss + KL divergence for VAE training.
"""

import torch
import torch.nn as nn
import torch.nn.functional as F
import config


class MultiScaleSpectralLoss(nn.Module):
    """Multi-scale spectral convergence + log magnitude loss."""

    def __init__(self, fft_sizes=None):
        super().__init__()
        self.fft_sizes = fft_sizes or config.SPECTRAL_FFT_SIZES

    def forward(self, x, x_hat):
        """
        Args:
            x: (batch, 1, 4096) original audio
            x_hat: (batch, 1, 4096) reconstructed audio
        Returns:
            loss: scalar, sum of spectral convergence + log magnitude across scales
        """
        total_loss = torch.tensor(0.0, device=x.device)
        x_flat = x.squeeze(1)
        x_hat_flat = x_hat.squeeze(1)

        for fft_size in self.fft_sizes:
            hop = fft_size // 4
            window = torch.hann_window(fft_size, device=x.device)

            X = torch.stft(x_flat, fft_size, hop, window=window,
                          return_complex=True)
            X_hat = torch.stft(x_hat_flat, fft_size, hop, window=window,
                              return_complex=True)

            X_mag = torch.abs(X)
            X_hat_mag = torch.abs(X_hat)

            # Spectral convergence
            sc = torch.norm(X_mag - X_hat_mag, p='fro') / (torch.norm(X_mag, p='fro') + 1e-7)
            total_loss = total_loss + sc

            # Log magnitude loss
            log_mag = F.l1_loss(torch.log(X_mag + 1e-7), torch.log(X_hat_mag + 1e-7))
            total_loss = total_loss + log_mag

        return total_loss / len(self.fft_sizes)


class TextureVAELoss(nn.Module):
    """Combined VAE loss: spectral + waveform L1 + beta * KL."""

    def __init__(self):
        super().__init__()
        self.spectral_loss = MultiScaleSpectralLoss()
        self.l1_weight = config.WAVEFORM_L1_WEIGHT

    def forward(self, x, recon, mu, logvar, beta):
        """
        Args:
            x: (batch, 1, 4096) original
            recon: (batch, 1, 4096) reconstruction
            mu: (batch, 32)
            logvar: (batch, 32)
            beta: float, KL weight from annealing schedule
        Returns:
            total_loss, spectral_loss, l1_loss, kl_loss (all scalars)
        """
        spec_loss = self.spectral_loss(x, recon)
        l1_loss = F.l1_loss(x, recon)
        kl_loss = -0.5 * torch.mean(1 + logvar - mu.pow(2) - logvar.exp())

        total = spec_loss + self.l1_weight * l1_loss + beta * kl_loss

        return total, spec_loss, l1_loss, kl_loss
